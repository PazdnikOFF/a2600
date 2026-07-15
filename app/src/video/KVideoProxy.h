#pragma once

#include <QObject>
#include <QImage>
#include <QString>
#if defined(HAVE_GST)
#include <gst/gst.h>   // видео-тракт V4L2/GStreamer — только сборка endostation (device)
#endif

class KPlControl;

// Прокси видео/камеры/FPGA. Имя и состав методов повторяют оригинальный класс
// KVideoProxy (реф. X-2000/X-2600): InitCamera, ResetVideo, FreezeVideoSwitch,
// GetSensorID, ReadWhbResult, RBCValueSet и т.д.
//
// В оригинале кадры идут FPGA→shm; здесь (реимплементация) источник — стандартный
// V4L2 (/dev/video0, NV12) через GStreamer, что эквивалентно на том же железе.
// Сигнал доставки кадра во вьювер — glue реимплементации.
class KVideoProxy : public QObject
{
    Q_OBJECT
public:
    struct Config {
        QString device        = "/dev/video0";
        int     width         = 1280;
        int     height        = 960;
        bool    useTestSource = false;
    };

    explicit KVideoProxy(QObject *parent = nullptr);
    ~KVideoProxy() override;

    void AttachPl(KPlControl *pl) { pl_ = pl; } // доступ к регистрам FPGA

    // --- имена методов из оригинального KVideoProxy ---
    bool InitCamera(const Config &cfg);   // init сенсора (PL-регистры) + запуск тракта
    void ResetVideo();                    // остановка/сброс
    void FreezeVideoSwitch(bool freeze);  // стоп-кадр
    bool IsSoftEndo() const { return true; } // гибкий эндоскоп (X-2600)
    int  GetSensorID() const { return sensorId_; }

    // Снимок: имя файла (реф. GenerateVideoFileName — timestamp) и сохранение JPEG
    // текущего кадра (реф. ImageSavePreset). Путь = <data>/<пациент>_<осмотр>/N.jpeg.
    QString GenerateVideoFileName() const;   // ГГГГММДДЧЧММСС
    bool    ImageSavePreset(const QString &dir, int index);
    QImage  CurrentFrame() const { return lastFrame_; }

    // Загрузка алгопараметров сенсора (гамма/CCM) в FPGA через KPlControl.
    // AlgParaManager вычисляет LUT/матрицу → KPlControl::SetGammaLut/SetCCM0Matrix.
    void ApplyImageParams(const QString &sensor, const QString &res,
                          const QString &scope);

    // Col RBC (реф. RBCValueSet) — режим + усиления R/B/S через KVideoParam,
    // запись гейнов в PL (SetColorR/B/C, регистры 0xa1870000).
    void RBCValueSet(int mode, int value);

    // Цветоусиление (реф. SendColorEnhanceValue): 0 — выкл., иначе вкл.+уровень.
    void SendColorEnhanceValue(int level);
    // Усиление изображения (реф. SendImageEnhanceValue): уровень → PL.
    void SendImageEnhanceValue(int level);
    // Уровень Bright EQ (реф. SetBrightEQLevel/SetBrightnessEQLevel).
    void SetBrightEQLevel(int level);

    // Экспозиция/усиление (реф. SetAECValue/SetAGCValue). Ветвление по режиму
    // тракта AE (поле реф. [this+0x24]): ==2 — прямая запись в сенсор по FPGA-I2C
    // (SetCameraAEC/AGCValue); иначе — через PL-регистр AE-блока (SetEndoAEC/AGCValue).
    // Дедуп с keep-alive (реф. static repeatCnt): при изменении значения — запись+сброс
    // счётчика; при том же значении запись повторяется первые 2 вызова и затем
    // освежается каждый 190-й (repeatCnt>0xbc) — AE-цикл зовёт их периодически.
    void SetAECValue(unsigned int aec);
    void SetAGCValue(unsigned int agc);
    void SetAECAndAGCValue(unsigned int aec, unsigned int agc); // реф. дедуп+запись в PL
    void SetAecAgcRouteMode(int mode) { aeRouteMode_ = mode; }  // 2=камера(I2C), иначе=PL
    // Конвертер выдержки (мс) → код AEC сенсора (реф. getAecValue, имя со строчной).
    // Формула зависит от режима тракта ([this+0x24]): 0/1/3 — линейные масштабы,
    // 2 — инверсная (код = 2307 − (t·72000−112)/520); неизвестный режим → лог + 10.
    unsigned int getAecValue(float value);

    // Скейлер стоп-кадра (реф. SetFreezeCalResolution): вход (w,h) → окно PIP из
    // layout-ini (KDisplayOption [VIDEO]/IMAGE_PIP): ScalerIn/Out/Ratio(Q5.8)+VideoLoc.
    void SetFreezeCalResolution(int width, int height);

    // Шумоподавление (реф. SetImageDenoiseLevel/SetImgDenoiseLevel): уровень→LUT+рег.
    void SetImageDenoiseLevel(int level);
    // Режим работы WL/EWL/SFI/VIST (реф. SetOperationMode): выбор конфигов + VIST-тракт.
    void SetOperationMode(int mode);
    // Применить VIST/SFI-матрицу текущего режима (реф. GetVistValue→SetVistMatrix).
    void ApplyVistMatrix();

    // --- Тонкие обёртки-оркестраторы к KPlControl (реф. 1:1, имена как в бинарнике,
    //     включая оригинальные опечатки SetColroCLevel/SetVideoDisPlay) ---
    void SetColorRLevel(int level);   // → pl_->SetColorR
    void SetColorBLevel(int level);   // → pl_->SetColorB
    void SetColroCLevel(int level);   // → pl_->SetColorC  (реф.: опечатка Colro)
    void SetCutPara(int a, int b);    // → pl_->SetCutPara
    void SetRealtimeVideoState(int state);  // → pl_->SetRealtimeVideoState
    void SetVideoDisPlay(int mode);   // → pl_->SetVideoDisplay (реф.: опечатка DisPlay)
    // Зеркало/поворот — команд. последовательности в FPGA-I2C рег. камеры (реф.).
    void SetHorizontalMirror(int mode);  // mode==4: 2 команды в REG_CAM_CMD
    void SetRotateType(int type);        // type==2: 3 команды в REG_CAM_CMD + паузы
    // Управление монитором (реф. SetMonitorCtrl): REG_MONITOR_CTRL=((v·100000)<<4)|3.
    void SetMonitorCtrl(unsigned int value);
    // Чтение произвольного PL-регистра (реф. GetPLRegisterValue) → ReadValueFromPL.
    unsigned int GetPLRegisterValue(unsigned int addr);

    // --- Конвертеры фиксированной точки и клампы (реф., 1:1 с дизасмом) ---
    // Float2FixedPointNumber(f, a, b): Q(a).(b) с насыщением ±(2^(a+b)−1); scale=2^b.
    int    Float2FixedPointNumber(float f, int a, int b);
    // FixedPointNumber2Float(x): Σ bit_i·2^(i−12), i=0..11 = (x&0xfff)/4096.
    double FixedPointNumber2Float(unsigned int x);
    // IncreaseValue(v, max): v = min(v+1, max). DecreaseValue(v): v = max(v−1, 0).
    void   IncreaseValue(int &value, int maxValue);
    void   DecreaseValue(int &value);

private:
    void SetCameraAECValue(unsigned int aec);  // FPGA-I2C: 0xa0048074/70, cmd 0xc00/0xd00
    void SetCameraAGCValue(unsigned int agc);  // FPGA-I2C: cmd 0xa00/0xb00
    void SetEndoAECValue(unsigned int aec);    // → SetAECAndAGCValue(aec, cachedAGC)
    void SetEndoAGCValue(unsigned int agc);    // → SetAECAndAGCValue(cachedAEC, agc)
public:

#if defined(HAVE_GST)
    bool isRunning() const { return pipeline_ != nullptr; }
#else
    bool isRunning() const { return false; }   // без gst видео-тракта нет
#endif
    const Config &config() const { return cfg_; }

signals:
    void VideoFrameReady(const QImage &frame); // доставка кадра во вьювер
    void VideoError(const QString &message);

private:
    void InitSensorRegs();   // PL-регистровая init-последовательность (из дизасма)
    bool StartCapture();     // запуск V4L2/GStreamer тракта

#if defined(HAVE_GST)
    static GstFlowReturn onNewSample(GstElement *sink, gpointer user);
    static gboolean      onBusMessage(GstBus *bus, GstMessage *msg, gpointer user);
    void handleSample(GstSample *sample);
#endif

    Config      cfg_;
    KPlControl *pl_         = nullptr;
#if defined(HAVE_GST)
    GstElement *pipeline_   = nullptr;
    GstElement *appsink_    = nullptr;
    guint       busWatchId_ = 0;
#endif
    bool        frozen_     = false;
    int         sensorId_   = 0;
    QImage      lastFrame_;   // последний кадр (для снимка/стоп-кадра)

    // Кэш последних значений AE (реф. поля KVideoProxy: AEC@+0x30, AGC@+0x2c)
    // для дедупа записей; режим тракта AE (реф. @+0x24).
    unsigned int cachedAEC_  = 0;
    unsigned int cachedAGC_  = 0;
    int          aeRouteMode_ = 2;   // по умолчанию камера/I2C (сенсоры с рег. экспозиции)

    // Текущий контекст сенсора/эндоскопа/режима (для загрузки конфигов VIST/Denoise/Awb).
    QString curSensor_ = "IMX274";
    QString curRes_    = "1920X1080";
    QString curScope_;
    int     operationMode_ = 0;      // WL/EWL/SFI/VIST (AlgParaManager::OperationMode)
    int     denoiseLevel_  = 1;
};
