#include "video/KVideoProxy.h"
#include "ctrl/KPlControl.h"
#include "alg/AlgParaManager.h"
#include "video/KVideoParam.h"

#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <gst/app/gstappsink.h>
#include <gst/video/video.h>

#if defined(__linux__)
#include <unistd.h>   // usleep
#else
static inline void usleep(unsigned) {}
#endif

KVideoProxy::KVideoProxy(QObject *parent) : QObject(parent)
{
    if (!gst_is_initialized())
        gst_init(nullptr, nullptr);
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
}

void KVideoProxy::ResetVideo()
{
    if (busWatchId_) { g_source_remove(busWatchId_); busWatchId_ = 0; }
    if (appsink_)    { gst_object_unref(appsink_); appsink_ = nullptr; }
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
    }
}

void KVideoProxy::FreezeVideoSwitch(bool freeze)
{
    frozen_ = freeze; // при заморозке кадры из appsink игнорируются
}

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
    // Реф.: писать в PL только при изменении пары (дедуп по кэшу).
    if (cachedAGC_ == agc && cachedAEC_ == aec)
        return;
    cachedAEC_ = aec;
    cachedAGC_ = agc;
    if (pl_) pl_->SetAECAndAGCValue(aec, agc);
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
    // Реф. SetAECValue: aeRouteMode_==2 → камера (I2C); иначе → PL AE-блок.
    if (aeRouteMode_ == 2) {
        if (cachedAEC_ == aec) return;    // дедуп
        cachedAEC_ = aec;
        SetCameraAECValue(aec);
    } else {
        SetEndoAECValue(aec);
    }
}

void KVideoProxy::SetAGCValue(unsigned int agc)
{
    // Реф. SetAGCValue: aeRouteMode_==2 → камера (I2C); иначе → PL AE-блок.
    if (aeRouteMode_ == 2) {
        if (cachedAGC_ == agc) return;    // дедуп
        cachedAGC_ = agc;
        SetCameraAGCValue(agc);
    } else {
        SetEndoAGCValue(agc);
    }
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
