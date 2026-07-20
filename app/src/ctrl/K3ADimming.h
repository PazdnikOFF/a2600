#pragma once

#include <cstdint>

// Авто-диммирование 3A (реф. K3ADimming, X-2600) — экспозиция / ток лампы / AGC.
//
// РЕВЕРС: синглтон (Instance() — ленивое `new` БЕЗ guard'а), ни vtable, ни
// typeinfo; sizeof == 0x288 (648).
//
// ⚠️ ВАЖНО ДЛЯ ПОНИМАНИЯ ГРАНИЦЫ: яркость здесь НЕ ВЫЧИСЛЯЕТСЯ. Ни гистограммы,
// ни зонного взвешивания, ни таблиц весов в классе нет — статистика приходит
// УЖЕ ГОТОВОЙ (два uint32 + два double) в AUTO_DIMMING_PARA от
// KVideoProxy::ReadIrisAndRatio через KSystemStatus. Поэтому синтетические
// входы self-test'а — ровно то же, что видит боевой код.
//
// Единственная реальная работа с железом во всём классе — одна запись
// KPlControl::WriteValueToPL в SetOV6946DimmingParam.

// Тип сенсора (индексы 0..4 из switch'ей; имена 3 и 4 закреплены
// SetSensorLgtParam, остальные три в бинарнике не именованы — НЕ УСТАНОВЛЕНО).
enum _SensorType {
    SENSOR_TYPE_0 = 0,
    SENSOR_TYPE_1 = 1,
    SENSOR_TYPE_2 = 2,
    SENSOR_TYPE_OV6946 = 3,
    SENSOR_TYPE_OAH0428 = 4,
};

// Вход шага диммирования (реф. AUTO_DIMMING_PARA, 24 байта).
struct AUTO_DIMMING_PARA {
    uint32_t target = 0;     // 0x00 целевая яркость (GetIrisTargetValue() & 0xFF)
    uint32_t measured = 0;   // 0x04 измеренная (KSystemStatus[0x5c])
    double   d8 = 0.0;       // 0x08 ratio1 (KSystemStatus[0x60])
    double   d10 = 0.0;      // 0x10 ratio2 (KSystemStatus[0x68])
};

// Результат шага (реф. AUTO_DIMMING_RESULT).
struct AUTO_DIMMING_RESULT {
    int agc = 0;
    int alc = 0;
    int aec = 0;
};

class K3ADimming
{
public:
    static K3ADimming *Instance();

    // --- главный шаг --------------------------------------------------------
    // ⚠️ КВИРК: при measured == 0 ЗАПИСЫВАЕТ 1 ОБРАТНО в структуру вызывающего.
    // ⚠️ Мёртвая зона: |target - measured| <= m_deadZone (15) ⇒ выход без изменений.
    void Calculate3ADimmingPara(AUTO_DIMMING_PARA &p);
    void GetDimmingResult(AUTO_DIMMING_RESULT &r);

    // --- математика ---------------------------------------------------------
    static double CalLog(double x);   // 20*log10(x)
    static double CalPow(double x);   // pow(10, x/20)
    // ⚠️ Ассиметрия: x >= 0 возвращается как есть; при x < 0 умножается на
    // (-0.8*r^2 + 1.8*r + 0.5), где r = m_ratio1. При r == 0 это 0.5 —
    // затемнение идёт ВДВОЕ медленнее осветления.
    double exposureRatioCal(double x);
    // Подавление пересветов: m_deltK * pow(m_deltBase, m_deltExpScale*d8)
    //                        * CalLog(255/target) - m_deltW2 * d10.
    double CalDelt(const AUTO_DIMMING_PARA &p);
    // ⚠️ ПИД в ПРИРАЩЕНИЯХ (velocity form). История eLast/ePre — ФАЙЛОВЫЕ
    // СТАТИКИ типа float, НЕ поля объекта: переживают пересоздание синглтона и
    // общие для всех. Округление до float воспроизведено намеренно.
    // ⚠️ Ни ограничения интеграла (windup), ни клампа выхода НЕТ.
    double CalculatePidOut(double e);
    // Выбор набора коэффициентов по полосе m_lgtTotal и знаку ошибки.
    // ⚠️ Ветка L > p3 выбирает ТОТ ЖЕ набор, что и L <= p2 — сравнение с p3
    // лишь вырезает среднюю полосу (мёртвая ветка).
    void UpdataPid(double e);
    // Единственный кламп интегратора.
    double UpdataLgtData(double lgt);

    // --- преобразования свет/ток/регистры -----------------------------------
    int    ldb2Lp(double dB);        // дБ лампы -> ток
    double lp2LdB(int lp);           // ток -> дБ. ⚠️ Домен не проверяется: при
                                     // lp <= B получится log10(<=0) → -inf/NaN.
    double Conver3ADimmingParaToLgt();
    void   ConverLgtTo3ADimmingPara(double L);
    unsigned ExposureTimeToRegisterValue(double t);
    unsigned gdbRegisterValue(double dB);

    // --- настройка ----------------------------------------------------------
    void SetEndoSensorType(int t);
    void SetDimmingSpeed(double v);
    void SetLightCurrentRange(int cur);
    void SetAGCStatus(int st);
    void UpdateAlcMin(int v);
    // ⚠️ КВИРК: НИЧЕГО НЕ СОХРАНЯЕТ — всё тело метода это лог. Мёртвый сеттер.
    void SetAlcMax(double v);
    // Наборы ПИД + пороги из _KAutomaticDimmerParam (ctrl/KColdLightConfig.h).
    // ⚠️ КВИРК: поля 0x54/0x58 присваиваются ПЕРЕКРЁСТНО (0x54→m_deltExpScale,
    // 0x58→m_deltW2) — та же перестановка, что отмечена со стороны ini.
    void SetDimmingPidParam(const struct _KAutomaticDimmerParam &p);

    // Пресеты AUTO_DIMMING_PARA (p0..p5).
    void SetCameraAutoDimmingParam();
    void SetCameraManuDimmingLgtRange();
    void SetOAH0428DimmingParam();
    // ⚠️ Байт-клон SetCameraAutoDimmingParam, НО m_expMax не выставляет.
    void SetOtherSensorDimmingParam();
    // Единственный device-метод: WriteValueToPL(0xA0048010, 0x00350303).
    void SetOV6946DimmingParam();

    // ⚠️ Без проверки границ ОБОИХ индексов.
    int  GetManuDimmingALC(int mode, int level);
    bool GetAlcValidState() const { return m_alcValid; }

    // --- доступ для self-test (не из реф.) ----------------------------------
    double LgtTotal() const   { return m_lgtTotal; }
    void   SetLgtTotal(double v) { m_lgtTotal = v; }
    double AecLgt() const     { return m_aecLgt; }
    double AlcLgt() const     { return m_alcLgt; }
    double AgcLgt() const     { return m_agcLgt; }
    double ExposureTime() const { return m_exposureTime; }
    int    LightCurrent() const { return m_lightCurrent; }
    double Gain() const       { return m_gain; }
    int    AlcMin() const     { return m_alcMin; }
    int    AlcMax() const     { return m_alcMax; }
    void   SetRatio1(double v) { m_ratio1 = v; }
    const double *Preset() const { return m_para; }
    // Сброс статиков ПИД-истории (в реф. они файловые и не сбрасываются никогда —
    // нужно только для детерминизма теста).
    static void ResetPidHistory();
    // Шов вместо GetSystemStatus()-гейтов (в реф. читаются только флаги/индексы).
    static void SetGateLampDisabled(bool v);   // status[0x80] != 0
    void   Reset();                            // состояние ctor

private:
    K3ADimming();

    int      m_sensorType = 0;          // 0x00
    double   m_lgtTotal = 0.0;          // 0x08 интегратор, дБ
    double   m_dimmingSpeed = 1.0;      // 0x10
    int8_t   m_agcOnHeadroom = 9;       // 0x18
    int8_t   m_agcOffHeadroom = -6;     // 0x19
    double   m_aecLgt = 0.0;            // 0x20
    double   m_alcLgt = 0.0;            // 0x28
    double   m_agcLgt = 0.0;            // 0x30
    double   m_exposureTime = 16.5;     // 0x38
    double   m_unused40 = 0.1;          // 0x40 ⚠️ пишется ctor, НЕ ЧИТАЕТСЯ НИГДЕ
    double   m_expMax = 16.5;           // 0x48
    int      m_lightCurrent = 33060;    // 0x50
    int      m_alcMin = 33060;          // 0x54
    int      m_alcMax = 47514;          // 0x58
    double   m_gain = 1.0;              // 0x60
    double   m_gainDefault = 1.0;       // 0x68
    uint8_t  m_deadZone = 15;           // 0x70
    float    m_deltK = -0.1f;           // 0x74
    float    m_deltBase = 1.8f;         // 0x78
    float    m_deltW2 = 1.0f;           // 0x7c
    float    m_deltExpScale = 4.0f;     // 0x80
    float    m_pidBypassThresh = 0.25f; // 0x84
    float    m_kp = 0.0f;               // 0x88
    float    m_ki = 0.0f;               // 0x8c
    float    m_kd = 0.0f;               // 0x90
    float    m_pidSets[6][3] = {};      // 0x94 6 троек {Kp,Ki,Kd}
    double   m_para[6] = {};            // 0xe0 пресет p0..p5
    int      m_agcStatus = 1;           // 0x110
    unsigned m_lastCur = 170;           // 0x118 только для лога
    unsigned m_lastTgt = 0;             // 0x11c только для лога
    double   m_ratio1 = 0.0;            // 0x120
    double   m_ratio2 = 0.0;            // 0x128
    int      m_manuAlcTable[4][19] = {};// 0x130
    unsigned m_alcRange[8] = {};        // 0x260
    bool     m_alcValid = false;        // 0x280
};
