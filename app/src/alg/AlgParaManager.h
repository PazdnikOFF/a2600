#pragma once

#include <QString>
#include <QVector>

// Менеджер алгопараметров обработки изображения (реф. класс AlgParaManager, X-2600).
// Синглтон (реф. GetInstance/pAlgParaManager). Загружает параметры из videoconf/
// (Gamma, Ccm, ColEnh, Denoise, Awb, Bright_EQ, CHb, Knee, ImgEnh) per сенсор/эндоскоп
// и вычисляет LUT/коэффициенты, которые применяются в PL (FPGA) через KPlControl.
//
// Реальные методы: GetInstance, CalGammaLut, LoadColEnhPara, LoadCcmPara,
// LoadDenoisePara, LoadBrightEqPara, LoadAwbWLPara/EWLPara, GetVistValue,
// GetSensorConfigFile/GetEndoConfigFile.
class AlgParaManager
{
public:
    static AlgParaManager &GetInstance();

    struct GammaParam { double bp = 0.001; double gamma = 2.2; int inputmax = 1024; };

    // Загрузка гамма-параметров: videoconf/Gamma/<SENSOR>_GammaParam[_<SCOPE>].ini
    GammaParam LoadGammaPara(const QString &sensor, const QString &scope = QString()) const;

    // Вычисление гамма-LUT (реф. CalGammaLut) — кривая с чёрной точкой bp.
    // Возвращает LUT размера inputmax (значения 0..inputmax-1) для загрузки в PL.
    static QVector<int> CalGammaLut(const GammaParam &p);

    // Текущий размер видео-области (реф. AlgParaManager::resize). Вызывается из
    // KPlControl::SetVideoArea; downstream-параметры считаются от этих размеров.
    void resize(int width, int height) { videoW_ = width; videoH_ = height; }
    int VideoWidth() const { return videoW_; }
    int VideoHeight() const { return videoH_; }

    // Путь конфига параметров для сенсора (реф. GetSensorConfigFile).
    QString GetSensorConfigFile(const QString &category, const QString &sensor) const;

    // Загрузка цветоусиления per сенсор (реф. LoadColEnhPara) — заглушка чтения папки.
    bool LoadColEnhPara(const QString &sensor);

    // Матрица цветокоррекции 3×3 (реф. LoadCcmPara). Значения Q9 (1.0 = 512),
    // знаковые 16-бит, из videoconf/Ccm/V1/<SENSOR>_<WxH>_<SCOPE>.txt.
    struct CcmMatrix { int m[9] = {512,0,0, 0,512,0, 0,0,512}; bool valid = false; };
    CcmMatrix LoadCcmMatrix(const QString &sensor, const QString &res,
                            const QString &scope) const;

    // Цветоусиление-LUT (реф. ColEnh) из colenh_para.txt.
    QVector<int> LoadColEnhLut(const QString &sensor) const;

    // --- Таблицы уровней параметров изображения (кэш для KPlControl) ---
    // Оригинал держит их массивами внутри AlgParaManager; SetColorEnhParam/
    // SetImageEnhValue берут значение по индексу уровня.

    // colenh_level.txt (hex, по строке на уровень) → массив значений ColorEnh.
    void LoadColEnhLevels(const QString &sensor);
    int  ColEnhLevelValue(int level) const;   // 0 если уровень вне диапазона

    // ImgEnh/<mode>/<subdir>/level_<ch>.txt (hex, по строке на уровень).
    // subdir — напр. "OV2740_EG_1280X960"; ch — 'a'/'b'/'e' (яркость/цвет/край).
    void LoadImgEnhLevels(const QString &mode, const QString &subdir, char ch = 'a');
    int  ImgEnhLevelValue(int level) const;

    // Режим работы (реф. enum, каталоги videoconf): WL/EWL/SFI/VIST.
    // Числовые значения соответствуют GetSystemStatus[0x3c]: 2=SFI, 3=VIST.
    enum OperationMode { WL = 0, EWL = 1, SFI = 2, VIST = 3 };
    static QString ModeDir(int mode);   // "WL"/"EWL"/"SFI"/"VIST"

    // VIST/SFI-матрица (реф. LoadVistPara/GetVistValue): 9 значений (3×3),
    // из Vist/V1/<MODE>/<SENSOR>_<res>.txt. mode ∈ {SFI,VIST}.
    QVector<unsigned> LoadVistMatrix(int mode, const QString &sensor,
                                     const QString &res) const;

    // AWB-гейны per режим/эндоскоп (реф. Awb): Awb/V1/<MODE>/<SENSOR>_<res>[_<SCOPE>].txt.
    // 6 значений (реф. порядок файла). Питают KPlControl::SetAWBValue/SetAwbCut.
    QVector<unsigned> LoadAwbGains(int mode, const QString &sensor,
                                   const QString &res, const QString &scope = QString()) const;

    // Denoise (реф. LoadDenoisePara): Denoise/<MODE>/<SENSOR>_Denoise[_<SCOPE>].ini.
    // Секции [denoiseLevel_N]: dpc_thd_t1/t2, kernelG(42), kernelRB(25), Lut_0..15(256).
    struct DenoiseParam {
        int dpcT1 = 0, dpcT2 = 0;
        QVector<int> kernelG;    // 42 значения
        QVector<int> kernelRB;   // 25 значений
        QVector<int> lut;        // 256 значений (Lut_0..15 по 16)
        bool valid = false;
    };
    DenoiseParam LoadDenoisePara(const QString &sensor, int mode, int level,
                                 const QString &scope = QString()) const;

    // LUT линеаризации сенсора (реф. LoadSensorLut/SetSensorLut): 3 канала из
    // sensor_lut/<SENSOR>_<res>/sensor_lut_{r,g,b}.txt (по 1024 значения).
    struct SensorLut { QVector<unsigned> r, g, b; bool valid = false; };
    SensorLut LoadSensorLut(const QString &sensor, const QString &res) const;

    // RBC-LUT (реф. LoadRbcLut/SetRbcLut): 3 канала Hb/Hr/S из
    // rbc_lut/<SENSOR>_<res>/colrbc_{Hb,Hr,S}.txt (~30 значений/канал).
    struct RbcLut { QVector<unsigned> hb, hr, s; bool valid = false; };
    RbcLut LoadRbcLut(const QString &sensor, const QString &res) const;

    // Knee-LUT (реф. LoadKneePara/SetKneeLut): Knee/<SENSOR>_KneeLut[_<SCOPE>].txt
    // (hex, 1024 значения по 10 бит).
    QVector<int> LoadKneeLut(const QString &sensor, const QString &scope = QString()) const;

    // CHb (усиление гемоглобина, реф. LoadCHBPara): CHb/<SENSOR>_<res>.txt — 4 hex.
    // SetChbStatus пишет в PL 4-е значение (реф. AlgParaManager+0x7a3c).
    void LoadChbPara(const QString &sensor, const QString &res);
    int  ChbValue() const { return chbValue_; }   // 4-е значение (для 0xa1900018)

    // Таблица диафрагмы (реф. LoadIrisPara/SetIrisTable): IRIS/<SENSOR>_<res>[_<SCOPE>].txt
    // (8040 значений-масок 1/3/7 — план размеров апертуры).
    QVector<int> LoadIrisTable(const QString &sensor, const QString &res,
                               const QString &scope = QString()) const;

    // Обрезка углов (реф. SetCutCornerPara/SetRoundPara/SetOctagonPara). LUT из
    // 1080 значений (по строке кадра) — горизонтальный вырез угла. way 0 = круг
    // (радиус b, отступ c), way 1 = восьмиугольник (b=hi<<16|lo, c). Данные пишет
    // KPlControl::SetCornerCutWay в 0xa18c8000 (1080 слов). Размер выреза W(шир.)/H
    // задаётся конфигом кадра (реф. поля AlgParaManager+0x10/+0x14).
    static constexpr int kCutCornerLen = 1080;
    void SetCutCornerSize(int w, int h) { cutW_ = w; cutH_ = h; }
    int  CutCornerW() const { return cutW_; }
    int  CutCornerH() const { return cutH_; }
    // Вычислить LUT угла (реф. SetCutCornerPara): Reset(=W) → Round/Octagon.
    void SetCutCornerPara(int way, int b, int c);
    // Готовый LUT (way 0/1); пусто если не вычислен.
    const QVector<int> &CutCornerLut(int way) const;

    // Bright EQ (реф. LoadBrightEqPara): gaussian_filter_hex.txt (36 значений,
    // 15-бит) + lumaGainLut_<disable/low/middle/high>_hex.txt (~1024, 12-бит).
    // Данные грузятся в KPlControl::SetBrightEQLut (регистры 0xa1950004../8000..).
    void LoadBrightEqPara(const QString &sensor);
    const QVector<int> &BrightEqGaussian() const { return brightEqGauss_; }
    // idx: 0=disable, 1=low, 2=middle, 3=high.
    const QVector<int> &BrightEqLumaLut(int idx) const;

    // --- «Текущие» данные обработки, применяемые в PL ----------------------------
    // Реф.: KPlControl::SetGammaLut/SetKneeLut/SetRbcLut/SetIrisTable/SetDenoiseLut —
    // void-функции, читающие массив синглтона AlgParaManager. Здесь массив заменён
    // типизированными кэшами; оркестрация (KVideoProxy) заполняет их из конфигов
    // (Load*/CalGammaLut), затем зовёт void KPlControl::Set*, которые читают отсюда.
    struct DenoisePlData {                       // то, что SetDenoiseLut пишет в PL
        int dpc[4] = {0,0,0,0};                  // 4 значения → 0xa194x010
        QVector<int> kernelG, kernelRB, lut;     // банки ×4 (смежные окна)
    };
    void SetCurGammaLut(const QVector<int> &v)   { curGamma_ = v; }
    const QVector<int> &CurGammaLut() const       { return curGamma_; }
    void SetCurKneeLut(const QVector<int> &v)    { curKnee_ = v; }
    const QVector<int> &CurKneeLut() const        { return curKnee_; }
    void SetCurRbcLut(const RbcLut &v)           { curRbc_ = v; }
    const RbcLut &CurRbcLut() const               { return curRbc_; }
    void SetCurIrisTable(const QVector<int> &v)  { curIris_ = v; }
    const QVector<int> &CurIrisTable() const      { return curIris_; }
    void SetCurDenoise(const DenoisePlData &v)   { curDenoise_ = v; }
    const DenoisePlData &CurDenoise() const       { return curDenoise_; }
    void SetCurLensParam(int v)                  { curLensParam_ = v; }   // реф. AlgPara 0x7a40
    int  CurLensParam() const                     { return curLensParam_; }

private:
    AlgParaManager() = default;
    QVector<int> colEnhLevels_;    // значения ColorEnh по уровню
    QVector<int> imgEnhLevels_;    // значения ImageEnh по уровню
    QVector<int> brightEqGauss_;   // гауссов фильтр Bright EQ (36 значений)
    QVector<int> brightEqLuma_[4]; // lumaGainLut: disable/low/middle/high
    int videoW_ = 0, videoH_ = 0; // текущий размер видео-области (resize)
    int chbValue_ = 0;            // CHb: 4-е значение для 0xa1900018
    int cutW_ = 0, cutH_ = 0;     // corner-cut: ширина/высота выреза (реф. +0x10/+0x14)
    QVector<int> cutLut_[2];      // corner-cut LUT: way 0 круг / 1 восьмиугольник
    // «Текущие» данные для KPlControl::Set* (реф.: массив AlgParaManager).
    QVector<int> curGamma_, curKnee_, curIris_;
    RbcLut       curRbc_;
    DenoisePlData curDenoise_;
    int          curLensParam_ = 0;   // параметр линзы (реф. AlgPara 0x7a40)
};
