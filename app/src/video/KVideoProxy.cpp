#include "video/KVideoProxy.h"
#include "ctrl/KPlControl.h"
#include "ctrl/KPlRegs.h"
#include "alg/AlgParaManager.h"
#include "video/KVideoParam.h"
#include "ui/KDisplayOption.h"

#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <cmath>
#if defined(HAVE_GST)
#include <gst/app/gstappsink.h>
#include <gst/video/video.h>
#endif

#if defined(__linux__)
#include <unistd.h>   // usleep
#else
static inline void usleep(unsigned) {}
#endif

KVideoProxy::KVideoProxy(QObject *parent) : QObject(parent)
{
#if defined(HAVE_GST)
    if (!gst_is_initialized())
        gst_init(nullptr, nullptr);
#endif
    qRegisterMetaType<QImage>("QImage");
}

KVideoProxy::~KVideoProxy()
{
    ResetVideo();
}

bool KVideoProxy::InitCamera(const Config &cfg)
{
    ResetVideo();
    cfg_ = cfg;
    frozen_ = false;

    InitSensorRegs();   // как в оригинале: программирование сенсора через PL-регистры
    return StartCapture();
}

// Регистровая init-последовательность сенсора — воспроизведена op-в-op из
// дизассемблера KVideoProxy::InitCamera оригинала (регион PL 0xa004xxxx:
// 0x8070/0x8074 — FPGA-I2C data/trigger, 0xa020/0xa030 — управление).
void KVideoProxy::InitSensorRegs()
{
    if (!pl_) return; // десктоп/без /dev/mem — пропускаем
    unsigned int tmp = 0;

    pl_->WriteValueToPL(0xa0048080, 0x14);
    pl_->ReadValueFromPL(0xa0048084, tmp);
    pl_->ReadValueFromPL(0xa004a030, tmp);
    // W 0xa004a030 <значение из регистра — вычисляется по прочитанному>
    pl_->ReadValueFromPL(0xa004a030, tmp);
    pl_->ReadValueFromPL(0xa004a030, tmp);
    usleep(1000);
    pl_->WriteValueToPL(0xa0048074, 0x0);
    pl_->WriteValueToPL(0xa0048070, 0x1008);
    pl_->WriteValueToPL(0xa0048074, 0xd);
    pl_->WriteValueToPL(0xa0048070, 0x100c);
    pl_->WriteValueToPL(0xa004a020, 0xc);
    usleep(1000);
    pl_->WriteValueToPL(0xa0048074, 0x9);
    pl_->WriteValueToPL(0xa0048070, 0x100c);
    pl_->WriteValueToPL(0xa004a020, 0x8);
    pl_->WriteValueToPL(0xa004a020, 0x0);
    pl_->WriteValueToPL(0xa0048074, 0x0);
    pl_->WriteValueToPL(0xa0048070, 0x100c);
    // Прим.: часть операций оригинала адресуется через регистры (цикл по таблице
    // сенсора) — при углублении реверса развернуть полную таблицу.
}

bool KVideoProxy::StartCapture()
{
#if !defined(HAVE_GST)
    // Без GStreamer (сборка ui_preview на Mac) видео-тракт недоступен.
    return false;
#else
    // На приборе: V4L2 /dev/video0 (NV12, dmabuf), питаемый PL-видеотрактом
    // (switchvideoformat.sh). NV12→RGB для доставки во вьювер.
    QString src = cfg_.useTestSource
        ? QString("videotestsrc is-live=true")
        : QString("v4l2src device=%1 io-mode=dmabuf").arg(cfg_.device);

    const QString desc = QString(
        "%1 ! video/x-raw,format=NV12,width=%2,height=%3 ! "
        "videoconvert ! video/x-raw,format=RGB ! "
        "appsink name=sink emit-signals=true max-buffers=2 drop=true sync=false")
        .arg(src).arg(cfg_.width).arg(cfg_.height);

    GError *err = nullptr;
    pipeline_ = gst_parse_launch(desc.toUtf8().constData(), &err);
    if (!pipeline_ || err) {
        const QString msg = err ? QString::fromUtf8(err->message) : "gst_parse_launch failed";
        if (err) g_error_free(err);
        emit VideoError(QString("InitCamera: %1").arg(msg));
        ResetVideo();
        return false;
    }

    appsink_ = gst_bin_get_by_name(GST_BIN(pipeline_), "sink");
    g_signal_connect(appsink_, "new-sample", G_CALLBACK(onNewSample), this);

    GstBus *bus = gst_element_get_bus(pipeline_);
    busWatchId_ = gst_bus_add_watch(bus, onBusMessage, this);
    gst_object_unref(bus);

    if (gst_element_set_state(pipeline_, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        emit VideoError("InitCamera: не удалось перейти в PLAYING");
        ResetVideo();
        return false;
    }
    qInfo() << "KVideoProxy::InitCamera:" << desc;
    return true;
#endif
}

void KVideoProxy::ResetVideo()
{
#if defined(HAVE_GST)
    if (busWatchId_) { g_source_remove(busWatchId_); busWatchId_ = 0; }
    if (appsink_)    { gst_object_unref(appsink_); appsink_ = nullptr; }
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
    }
#endif
}

void KVideoProxy::FreezeVideoSwitch(bool freeze)
{
    frozen_ = freeze; // при заморозке кадры из appsink игнорируются
}

#if defined(HAVE_GST)
GstFlowReturn KVideoProxy::onNewSample(GstElement *sink, gpointer user)
{
    auto *self = static_cast<KVideoProxy *>(user);
    GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
    if (sample) {
        if (!self->frozen_)
            self->handleSample(sample);
        gst_sample_unref(sample);
    }
    return GST_FLOW_OK;
}

void KVideoProxy::handleSample(GstSample *sample)
{
    GstCaps *caps = gst_sample_get_caps(sample);
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    if (!caps || !buffer) return;

    GstVideoInfo info;
    if (!gst_video_info_from_caps(&info, caps)) return;

    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) return;

    const int w = GST_VIDEO_INFO_WIDTH(&info);
    const int h = GST_VIDEO_INFO_HEIGHT(&info);
    const int stride = GST_VIDEO_INFO_PLANE_STRIDE(&info, 0);

    QImage frame(reinterpret_cast<const uchar *>(map.data), w, h, stride, QImage::Format_RGB888);
    QImage copy = frame.copy();
    gst_buffer_unmap(buffer, &map);

    lastFrame_ = copy;              // для снимка/стоп-кадра
    emit VideoFrameReady(copy);
}
#endif // HAVE_GST

QString KVideoProxy::GenerateVideoFileName() const
{
    // Реф. KVideoProxy::GenerateVideoFileName — метка времени ГГГГММДДЧЧММСС.
    return QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
}

bool KVideoProxy::ImageSavePreset(const QString &dir, int index)
{
    // Реф. ImageSavePreset — снимок текущего кадра в JPEG (<dir>/<N>.jpeg).
    if (lastFrame_.isNull())
        return false;
    QDir().mkpath(dir);
    const QString path = QString("%1/%2.jpeg").arg(dir).arg(index);
    return lastFrame_.save(path, "JPEG", 90);
}

void KVideoProxy::RBCValueSet(int mode, int value)
{
    // Реф. RBCValueSet(mode,value): один канал за вызов. gain = base + value,
    // где base — общий базовый уровень тона (KVideoParam::RBCBase). Затем
    // SetRBCMode + сеттер гейна канала + запись гейна в PL (SetColorR/B/C):
    //   mode 0 → R (0xa1870004), 1 → B (0xa1870008), 2 → C/тон (0xa1870000).
    KVideoParam &vp = KVideoParam::Instance();
    const int gain = value + vp.RBCBase();
    vp.SetRBCMode(mode);
    switch (mode) {
    case 0: vp.SetRGain(gain); if (pl_) pl_->SetColorR(gain); break; // ColorRLevel
    case 1: vp.SetBGain(gain); if (pl_) pl_->SetColorB(gain); break; // ColorBLevel
    case 2: vp.SetSGain(gain); if (pl_) pl_->SetColorC(gain); break; // ColroCLevel
    default: break;
    }
}

void KVideoProxy::SendColorEnhanceValue(int level)
{
    // Реф. SendColorEnhanceValue: level==0 → выкл.; иначе вкл.+уровень.
    // Значение уровня резолвится внутри KPlControl из AlgParaManager.
    if (!pl_) return;
    if (level == 0)
        pl_->SetColorEnhParam(false, 0);
    else
        pl_->SetColorEnhParam(true, level);
}

void KVideoProxy::SendImageEnhanceValue(int level)
{
    // Реф. SendImageEnhanceValue → KPlControl::SetImageEnhValue.
    if (pl_) pl_->SetImageEnhValue(level);
}

void KVideoProxy::SetBrightEQLevel(int level)
{
    // Реф. SetBrightEQLevel(uchar): 0xff → следующий уровень по кругу [0..3];
    // иначе прямая установка. Включение EQ идёт по SetBrightEQEnable.
    KVideoParam &vp = KVideoParam::Instance();
    int lv = level;
    if (level == 0xff) {
        lv = vp.BrightEQ() + 1;
        if (lv >= 4) lv = 0;
    }
    vp.SetBrightEQ(lv);
    if (pl_) pl_->SetBrightEQEnable(lv > 0);
}

// --- Экспозиция (AEC) и усиление (AGC) ---
// Два тракта, выбор по aeRouteMode_ (реф. поле [this+0x24]):
//  • камера/I2C: значение пишется в регистр сенсора через FPGA-I2C-мост
//    (0xa0048074 data / 0xa0048070 trigger), два байта командами 0xc00/0xd00 (AEC)
//    и 0xa00/0xb00 (AGC), старший байт со взведённым битом 31 (0x80000000).
//  • PL AE-блок: пара (AEC,AGC) пишется одним регистром 0xa0048020 (SetAECAndAGCValue).

void KVideoProxy::SetCameraAECValue(unsigned int aec)
{
    if (!pl_) return;
    // Реф. SetCameraAECValue: младший байт (cmd 0xc00), затем старший (cmd 0xd00|бит31).
    pl_->WriteValueToPL(0xa0048074, (aec & 0xff) | 0xc00);
    usleep(200);
    pl_->WriteValueToPL(0xa0048070, 0x1014);              // строб записи
    usleep(200);
    pl_->WriteValueToPL(0xa0048074, ((aec >> 8) & 0xff) | 0xd00u | 0x80000000u);
    usleep(200);
    pl_->WriteValueToPL(0xa0048070, 0x1014);
}

void KVideoProxy::SetCameraAGCValue(unsigned int agc)
{
    if (!pl_) return;
    // Реф. SetCameraAGCValue: младший байт (cmd 0xa00), затем старшие 3 бита (cmd 0xb00|бит31).
    pl_->WriteValueToPL(0xa0048074, (agc & 0xff) | 0xa00);
    usleep(200);
    pl_->WriteValueToPL(0xa0048070, 0x1014);
    usleep(200);
    pl_->WriteValueToPL(0xa0048074, ((agc >> 8) & 0x7) | 0xb00u | 0x80000000u);
    usleep(200);
    pl_->WriteValueToPL(0xa0048070, 0x1014);
}

void KVideoProxy::SetAECAndAGCValue(unsigned int aec, unsigned int agc)
{
    // Реф.: дедуп по кэшу пары + keep-alive: при том же значении запись повторяется
    // первые 2 вызова (repeatCnt 1..2) и освежается каждый 190-й (repeatCnt>0xbc).
    static unsigned int repeatCnt = 0;
    if (cachedAGC_ != agc || cachedAEC_ != aec) {
        cachedAGC_ = agc;
        cachedAEC_ = aec;
        repeatCnt = 0;
        if (pl_) pl_->SetAECAndAGCValue(aec, agc);
        return;
    }
    const unsigned int prev = repeatCnt++;
    if (prev > 0xbc) {
        repeatCnt = 0;
        if (pl_) pl_->SetAECAndAGCValue(aec, agc);
    } else if (prev + 1 <= 2) {
        if (pl_) pl_->SetAECAndAGCValue(aec, agc);
    }
}

void KVideoProxy::SetEndoAECValue(unsigned int aec)
{
    SetAECAndAGCValue(aec, cachedAGC_);   // реф.: AGC берётся из кэша
}

void KVideoProxy::SetEndoAGCValue(unsigned int agc)
{
    SetAECAndAGCValue(cachedAEC_, agc);   // реф.: AEC берётся из кэша
}

void KVideoProxy::SetAECValue(unsigned int aec)
{
    // Реф. SetAECValue: aeRouteMode_==2 → камера (I2C) с дедупом+keep-alive
    // (свой static repeatCnt); иначе → PL AE-блок (SetEndoAECValue).
    if (aeRouteMode_ != 2) {
        SetEndoAECValue(aec);
        return;
    }
    static unsigned int repeatCnt = 0;
    if (cachedAEC_ != aec) {
        cachedAEC_ = aec;
        repeatCnt = 0;
        SetCameraAECValue(aec);
        return;
    }
    const unsigned int prev = repeatCnt++;
    if (prev > 0xbc) {
        repeatCnt = 0;
        SetCameraAECValue(aec);
    } else if (prev + 1 <= 2) {
        SetCameraAECValue(aec);
    }
}

void KVideoProxy::SetAGCValue(unsigned int agc)
{
    // Реф. SetAGCValue: зеркально SetAECValue (кэш AGC, свой repeatCnt).
    if (aeRouteMode_ != 2) {
        SetEndoAGCValue(agc);
        return;
    }
    static unsigned int repeatCnt = 0;
    if (cachedAGC_ != agc) {
        cachedAGC_ = agc;
        repeatCnt = 0;
        SetCameraAGCValue(agc);
        return;
    }
    const unsigned int prev = repeatCnt++;
    if (prev > 0xbc) {
        repeatCnt = 0;
        SetCameraAGCValue(agc);
    } else if (prev + 1 <= 2) {
        SetCameraAGCValue(agc);
    }
}

unsigned int KVideoProxy::getAecValue(float value)
{
    // Реф. getAecValue(float): выдержка (мс) → код AEC сенсора, формула по режиму
    // тракта AE ([this+0x24]). Константы 1:1 из дизасма (72/40 кГц·1000, делители).
    switch (aeRouteMode_) {
    case 0:
        return static_cast<unsigned int>(value * 72.0f * 1000.0f * 16.0f / 1080.0f);
    case 1:
        return static_cast<unsigned int>(value * 40.0f * 1000.0f / 794.0f);
    case 2:
        // Инверсная шкала (double-математика в реф.): код = 2307 − (t·72000 − 112)/520.
        return static_cast<unsigned int>(
            2307.0 - (static_cast<double>(value) * 72.0 * 1000.0 - 112.0) / 520.0);
    case 3:
        return static_cast<unsigned int>(value * 40.0f * 1000.0f / 765.0f);
    default:
        qWarning() << "getAecValue: unknown AE route mode" << aeRouteMode_;
        return 10;   // реф.: LogPrintfEx + return 10
    }
}

void KVideoProxy::SetFreezeCalResolution(int width, int height)
{
    // Реф. SetFreezeCalResolution(w,h): окно PIP стоп-кадра из layout-ini
    // (KDisplayOption::getFreezeVideoRect, [VIDEO]/IMAGE_PIP). Пустой rect
    // (x2−x1==−1 или y2−y1==−1, т.е. нулевая ширина/высота) → лог и выход.
    const QRect rect = KDisplayOption::Instance().getFreezeVideoRect();
    if (rect.width() == 0 || rect.height() == 0) {
        qWarning() << "SetFreezeCalResolution: invalid freeze video rect" << rect;
        return;
    }
    if (!pl_) return;
    pl_->SetFreezeScalerIn(width, height);
    pl_->SetFreezeScalerOut(rect.width(), rect.height());
    // Коэффициенты скейла вход/выход в фикс. точке Q5.8.
    const int ratioW = Float2FixedPointNumber(
        static_cast<float>(width) / static_cast<float>(rect.width()), 5, 8);
    const int ratioH = Float2FixedPointNumber(
        static_cast<float>(height) / static_cast<float>(rect.height()), 5, 8);
    pl_->SetFreezeScalerRatio(ratioW, ratioH);
    // Реф.: FreezeVideoLoc(x1, ширина, y1, высота).
    pl_->SetFreezeVideoLoc(rect.x(), rect.width(), rect.y(), rect.height());
}

void KVideoProxy::SetImgDenoiseLevel(int level)
{
    // Реф. SetImgDenoiseLevel (тонкий): рег. уровня + запоминание в KVideoParam.
    // (Полная перезагрузка LUT — в SetImageDenoiseLevel; здесь только рег. уровня.)
    if (pl_) pl_->SetDenoiseLevel(level);
    KVideoParam::Instance().SetDenoise(level);
}

void KVideoProxy::SetContrastLevel(int level)
{
    // Реф. SetContrastLevel(int): 0xff → следующий уровень по кругу [0..2]; контраст
    // входит в гамма-кривую (реф. AlgParaManager::UpdateGammaDownloadLut) → перезагрузка
    // гамма-LUT в PL. (Модуляция гаммы контрастом в CalGammaLut пока не реверснута —
    // здесь освежается текущая гамма, структура вызовов = дизасм.)
    KVideoParam &vp = KVideoParam::Instance();
    int lv = level;
    if (level == 0xff) {
        lv = vp.ContrastLevel() + 1;
        if (lv >= 3) lv = 0;
    }
    vp.SetContrastLevel(lv);
    if (pl_) pl_->SetGammaLut();
}

void KVideoProxy::SetBrightnessEQLevel(int level)
{
    // Реф. SetBrightnessEQLevel(int): level==0 → выкл EQ; иначе вкл + загрузка LUT
    // уровня. Запоминание уровня в KVideoParam. Клампа нет.
    if (level == 0) {
        if (pl_) pl_->SetBrightEQEnable(false);
    } else if (pl_) {
        pl_->SetBrightEQEnable(true);
        pl_->SetBrightEQLut(level);
    }
    KVideoParam::Instance().SetBrightEQ(level);
}

void KVideoProxy::SetLensSize(int a, int b)
{
    if (pl_) pl_->SetLensSize(a, b);   // реф. 1:1 → 0xa189000c = a|(b<<16)
}

void KVideoProxy::SetEnhanceSize(int a, int b)
{
    if (pl_) pl_->SetEnhanceSize(a, b);  // реф. 1:1 (в прошивке KPlControl-метод пустой)
}

void KVideoProxy::SetAwbCut(int low, int high)
{
    if (pl_) pl_->SetAwbCut(low, high);  // реф. 1:1 → 0xa1840018
}

void KVideoProxy::SendCHbLevel(int level)
{
    if (pl_) pl_->SetChbStatus(level);   // реф. 1:1
}

void KVideoProxy::SetDemoire()
{
    // Реф. SetDemoire(): тоггл по текущему статусу в KVideoParam.
    //  • статус==1 (вкл) → выключить: SetDemoire(0), SetDemoireEN(0), затем
    //    восстановить image-enhance текущего уровня (реф. GetImgEnhValue(lvl) —
    //    в нашей архитектуре SendImageEnhanceValue сам резолвит значение из AlgPara).
    //  • иначе → включить: SetDemoire(1), SetDemoireEN(1), image-enhance = 0.
    KVideoParam &vp = KVideoParam::Instance();
    if (vp.DemoireStatus() == 1) {
        vp.SetDemoire(0);
        if (pl_) pl_->SetDemoireEN(0);
        SendImageEnhanceValue(vp.ImageEnhLevel());
    } else {
        vp.SetDemoire(1);
        if (pl_) pl_->SetDemoireEN(1);
        SendImageEnhanceValue(0);
    }
}

void KVideoProxy::SetDehazeStatus(int status)
{
    // Реф. SetDehazeStatus: Dehaze не имеет своего PL-enable — модулирует гамма-кривую.
    // Взаимоисключение: включение Dehaze (status==1) гасит HDR. Затем статус в KVideoParam
    // + перезагрузка гамма-LUT (реф. AlgParaManager::UpdateGammaDownloadLut → SetGammaLut).
    // OSD-сообщение оригинала (KUiMsgProxy::DisplayMsg) здесь опущено (UI/device).
    KVideoParam &vp = KVideoParam::Instance();
    if (status == 1 && vp.HDRStatus() != 0)
        vp.SetHDR(0);
    vp.SetDehaze(status);
    if (pl_) pl_->SetGammaLut();
}

void KVideoProxy::SetHDRStatus(int status)
{
    // Реф. SetHDRStatus: зеркально SetDehazeStatus (включение HDR гасит Dehaze).
    KVideoParam &vp = KVideoParam::Instance();
    if (status == 1 && vp.DehazeStatus() != 0)
        vp.SetDehaze(0);
    vp.SetHDR(status);
    if (pl_) pl_->SetGammaLut();
}

void KVideoProxy::SetDehazeSwitch(int status)
{
    // Реф. SetDehazeSwitch(uchar): 0xff → следующий по кругу [0..1]; иначе прямо.
    int next = status;
    if (status == 0xff) {
        next = KVideoParam::Instance().DehazeStatus() + 1;
        if (next >= 2) next = 0;
    }
    SetDehazeStatus(next);
}

void KVideoProxy::SetHDRSwitch(int status)
{
    // Реф. SetHDRSwitch(uchar): 0xff → следующий по кругу [0..1]; иначе прямо.
    int next = status;
    if (status == 0xff) {
        next = KVideoParam::Instance().HDRStatus() + 1;
        if (next >= 2) next = 0;
    }
    SetHDRStatus(next);
}

void KVideoProxy::SetImageDenoiseLevel(int level)
{
    // Реф. SetImageDenoiseLevel: загрузить LUT уровня и записать регистр уровня.
    denoiseLevel_ = level;
    KVideoParam::Instance().SetDenoise(level);
    if (!pl_) return;
    AlgParaManager &alg = AlgParaManager::GetInstance();
    const auto dp = alg.LoadDenoisePara(curSensor_, operationMode_, level, curScope_);
    if (dp.valid) {
        // Реф.: конфиг → «текущий» массив AlgParaManager → void SetDenoiseLut читает его.
        AlgParaManager::DenoisePlData d;
        d.dpc[0] = d.dpc[1] = dp.dpcT1;
        d.dpc[2] = d.dpc[3] = dp.dpcT2;
        d.kernelG = dp.kernelG; d.kernelRB = dp.kernelRB; d.lut = dp.lut;
        alg.SetCurDenoise(d);
        pl_->SetDenoiseLut();
    }
    pl_->SetDenoiseLevel(level);
}

void KVideoProxy::ApplyVistMatrix()
{
    // Реф. GetVistValue→SetVistMatrix: для режимов SFI/VIST грузим матрицу и пишем в PL.
    if (!pl_) return;
    const bool on = (operationMode_ == AlgParaManager::VIST ||
                     operationMode_ == AlgParaManager::SFI);
    pl_->SetVistSwitch(on);
    if (!on) return;
    const QVector<unsigned> m =
        AlgParaManager::GetInstance().LoadVistMatrix(operationMode_, curSensor_, curRes_);
    if (!m.isEmpty())
        pl_->SetVistMatrix(m.constData(), m.size());
}

void KVideoProxy::SetColorRLevel(int level)
{
    // Реф. SetColorRLevel: pl_->SetColorR(level) (+ debug-лог в оригинале).
    if (pl_) pl_->SetColorR(level);
}

void KVideoProxy::SetColorBLevel(int level)
{
    if (pl_) pl_->SetColorB(level);
}

void KVideoProxy::SetColroCLevel(int level)
{
    // Реф. имя с опечаткой (Colro) — сохранено.
    if (pl_) pl_->SetColorC(level);
}

void KVideoProxy::SetCutPara(int a, int b)
{
    if (pl_) pl_->SetCutPara(a, b);
}

void KVideoProxy::SetRealtimeVideoState(int state)
{
    if (pl_) pl_->SetRealtimeVideoState(state);
}

void KVideoProxy::SetVideoDisPlay(int mode)
{
    // Реф. имя с опечаткой (DisPlay) — сохранено; → KPlControl::SetVideoDisplay.
    if (pl_) pl_->SetVideoDisplay(mode);
}

void KVideoProxy::SetHorizontalMirror(int mode)
{
    // Реф. SetHorizontalMirror: действует только при mode==4 — две команды в
    // FPGA-I2C регистр камеры REG_CAM_CMD (0xa0048010).
    if (mode != 4 || !pl_) return;
    pl_->WriteValueToPL(REG_CAM_CMD, 0x00370141);
    pl_->WriteValueToPL(REG_CAM_CMD, 0x00500703);
}

void KVideoProxy::SetRotateType(int type)
{
    // Реф. SetRotateType: только type==2 пишет — три команды в REG_CAM_CMD с паузами
    // (type==0 — ничего; иначе только debug-лог оригинала, без записей).
    if (type != 2 || !pl_) return;
    pl_->WriteValueToPL(REG_CAM_CMD, 0x003811a8);
    usleep(2000);
    pl_->WriteValueToPL(REG_CAM_CMD, 0x00381317);
    usleep(2000);
    pl_->WriteValueToPL(REG_CAM_CMD, 0x003820b0);
}

void KVideoProxy::SetMonitorCtrl(unsigned int value)
{
    // Реф. SetMonitorCtrl: REG_MONITOR_CTRL = ((value·100000)<<4) | 3 (32-бит).
    if (!pl_) return;
    pl_->WriteValueToPL(REG_MONITOR_CTRL, ((value * 100000u) << 4) | 3u);
}

unsigned int KVideoProxy::GetPLRegisterValue(unsigned int addr)
{
    // Реф. GetPLRegisterValue: ReadValueFromPL(addr) → значение (0 если чтение неуспешно).
    unsigned int v = 0;
    if (pl_) pl_->ReadValueFromPL(addr, v);
    return v;
}

int KVideoProxy::Float2FixedPointNumber(float f, int a, int b)
{
    // Реф. Float2FixedPointNumber (дизасм X2000): формат Q(a).(b), scale=2^b,
    // насыщение до потолка 2^(a+b)−1. Знак выносится наружу.
    const unsigned int ceiling = static_cast<unsigned int>(std::pow(2.0, a + b) - 1.0);
    const float scale = static_cast<float>(1u << b);   // 2^b
    if (f >= 0.0f) {
        const unsigned int v = static_cast<unsigned int>(scale * f);
        return static_cast<int>(qMin(v, ceiling));
    }
    const unsigned int v = static_cast<unsigned int>(-(f * scale));   // |f|·2^b
    return -static_cast<int>(qMin(ceiling, v));
}

double KVideoProxy::FixedPointNumber2Float(unsigned int x)
{
    // Реф. FixedPointNumber2Float (дизасм X2000): 12-бит дробь — сумма bit_i·2^(i−12)
    // по i=11..0 = (x&0xfff)/4096.
    double acc = 0.0;
    for (int bit = 11; bit >= 0; --bit)
        acc += static_cast<double>((x >> bit) & 1u) * std::pow(2.0, bit - 12);
    return acc;
}

void KVideoProxy::IncreaseValue(int &value, int maxValue)
{
    // Реф. IncreaseValue: value = min(value+1, maxValue).
    value = qMin(value + 1, maxValue);
}

void KVideoProxy::DecreaseValue(int &value)
{
    // Реф. DecreaseValue: value = max(value−1, 0).
    value = qMax(value - 1, 0);
}

void KVideoProxy::SetOperationMode(int mode)
{
    // Реф. SetOperationMode: сменить режим, обновить VIST-тракт и шумоподавление.
    operationMode_ = mode;
    KVideoParam::Instance().SetOperationMode(mode);
    ApplyVistMatrix();
    SetImageDenoiseLevel(denoiseLevel_);
}

void KVideoProxy::ApplyImageParams(const QString &sensor, const QString &res,
                                   const QString &scope)
{
    if (!pl_) return;
    curSensor_ = sensor;
    curRes_ = res;
    curScope_ = scope;
    AlgParaManager &alg = AlgParaManager::GetInstance();
    // Гамма: конфиг → «текущий» LUT AlgParaManager → PL (реф. UpdateGammaDownloadLut →
    // void SetGammaLut читает массив).
    const auto gp  = alg.LoadGammaPara(sensor, scope);
    alg.SetCurGammaLut(AlgParaManager::CalGammaLut(gp));
    pl_->SetGammaLut();
    // CCM: конфиг → матрица → PL (реф. SetCcmParam → SetCCM0Matrix(uint*, int)).
    const auto ccm = alg.LoadCcmMatrix(sensor, res, scope);
    if (ccm.valid)
        pl_->SetCCM0Matrix(reinterpret_cast<const unsigned int *>(ccm.m), 9);
    // Таблицы уровней параметров изображения (для Send*/SetBrightEQLut).
    alg.LoadColEnhLevels(sensor);
    alg.LoadBrightEqPara(sensor);
    alg.LoadChbPara(sensor, res);   // CHb-значение для SetChbStatus
    // LUT линеаризации сенсора → PL (реф. SetSensorLut → SetSensorR/G/BLut).
    const auto sl = alg.LoadSensorLut(sensor, res);
    if (sl.valid) {
        pl_->SetSensorRLut(sl.r.constData(), sl.r.size());
        pl_->SetSensorGLut(sl.g.constData(), sl.g.size());
        pl_->SetSensorBLut(sl.b.constData(), sl.b.size());
    }
    // RBC-LUT сосудистого контраста → «текущий» AlgParaManager → PL (реф. void SetRbcLut).
    const auto rb = alg.LoadRbcLut(sensor, res);
    if (rb.valid) {
        alg.SetCurRbcLut(rb);
        pl_->SetRbcLut();
    }
}

#if defined(HAVE_GST)
gboolean KVideoProxy::onBusMessage(GstBus *, GstMessage *msg, gpointer user)
{
    auto *self = static_cast<KVideoProxy *>(user);
    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR: {
        GError *err = nullptr; gchar *dbg = nullptr;
        gst_message_parse_error(msg, &err, &dbg);
        const QString text = QString("GStreamer: %1").arg(err ? err->message : "unknown");
        if (err) g_error_free(err);
        g_free(dbg);
        emit self->VideoError(text);
        break;
    }
    case GST_MESSAGE_EOS:
        emit self->VideoError("EOS");
        break;
    default: break;
    }
    return TRUE;
}
#endif // HAVE_GST
