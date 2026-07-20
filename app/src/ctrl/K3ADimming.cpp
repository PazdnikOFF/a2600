#include "ctrl/K3ADimming.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "ctrl/KColdLightConfig.h"   // _KAutomaticDimmerParam
#include "ctrl/KPlControl.h"
#include "kernel/KSystemLog.h"

// ⚠️ В бинарнике Float2FixedPointNumber продублирован ЧЕТЫРЕЖДЫ (K3ADimming
// @0x6b0058, KVideoProxy @0x6da040, AlgParaManager @0x3ebab8, KAlgParamAjustDlg
// @0x5ca490) — у каждого класса своя копия. Здесь тоже своя: это верно
// оригиналу и не тянет QObject-зависимость ради чистой функции.
// Идентична KVideoProxy::Float2FixedPointNumber (self-test `plreg`).

namespace {

// ⚠️ РЕФ.: eLast/ePre — ФАЙЛОВЫЕ СТАТИКИ типа float (.bss 0x14bfdc8/0x14bfdcc),
// а НЕ поля объекта: переживают пересоздание синглтона и общие для всех.
float g_eLast = 0.0f;
float g_ePre  = 0.0f;

// Шов вместо GetSystemStatus()[0x80] (в реф. — флаг «лампа недоступна»).
bool g_gateLampDisabled = false;

// Магический делитель пересчёта дБ лампы в ток (0x40D17BD47AE147AE).
const double kLampDivisor = 17903.32;

// Пресеты AUTO_DIMMING_PARA (p0..p5) из .rodata.
const double kPresetCameraAuto[6] = { -20.0, 24.3496, 24.3496, 69.4566, 69.4566, 96.4566 };
const double kPresetOV6946[6]     = { -20.0, 29.24,   29.24,   29.24,   29.24,   59.24   };
const double kPresetCameraManu[6] = { -20.0, 24.3496, 24.3496, 24.3496, 24.3496, 54.3496 };
const double kPresetOAH0428[6]    = {  20.0, 29.24,   29.24,   74.347,  74.347,  94.347  };

// Ctor-умолчания ПИД (18 float из .rodata 0x88cf08), тройки {Kp,Ki,Kd}.
const float kPidDefaults[6][3] = {
    { 0.0f, 0.5f, 0.0f },   // set0
    { 1.0f, 1.5f, 0.0f },   // set1
    { 0.0f, 0.5f, 0.0f },   // set2
    { 1.0f, 2.0f, 0.0f },   // set3
    { 0.0f, 0.5f, 0.0f },   // set4
    { 0.0f, 0.5f, 0.0f },   // set5
};

// m_manuAlcTable[4][19] из .rodata 0x88cdd8. Строки 1 и 2 ИДЕНТИЧНЫ (так в реф.).
const int kManuAlcTable[4][19] = {
    { 33259, 34050, 34842, 35634, 36426, 37218, 38010, 38802, 39594, 40386,
      41177, 41969, 42761, 43553, 44345, 45137, 45929, 46721, 47513 },
    { 34054, 34802, 35549, 36297, 37045, 37793, 38540, 39288, 40036, 40784,
      41531, 42279, 43026, 43774, 44522, 45270, 46017, 46765, 47513 },
    { 34054, 34802, 35549, 36297, 37045, 37793, 38540, 39288, 40036, 40784,
      41531, 42279, 43026, 43774, 44522, 45270, 46017, 46765, 47513 },
    { 33381, 34166, 34951, 35736, 36521, 37306, 38092, 38876, 39662, 40447,
      41232, 42017, 42802, 43587, 44372, 45157, 45942, 46727, 47513 },
};

// Q(a).(b) с насыщением +-(2^(a+b)-1), scale = 2^b.
int Float2FixedPointNumber(float f, int a, int b)
{
    const unsigned ceilv = unsigned(std::pow(2.0, a + b) - 1.0);
    const float scale = float(1 << b);
    if (f >= 0.0f)
        return int(std::min(unsigned(scale * f), ceilv));
    return -int(std::min(ceilv, unsigned(-(f * scale))));
}

} // namespace

void K3ADimming::ResetPidHistory() { g_eLast = 0.0f; g_ePre = 0.0f; }
void K3ADimming::SetGateLampDisabled(bool v) { g_gateLampDisabled = v; }

K3ADimming::K3ADimming()
{
    Reset();
}

void K3ADimming::Reset()
{
    m_sensorType = 0;
    m_lgtTotal = 0.0;
    m_dimmingSpeed = 1.0;
    m_agcOnHeadroom = 9;
    m_agcOffHeadroom = -6;
    m_aecLgt = m_alcLgt = m_agcLgt = 0.0;
    m_exposureTime = 16.5;
    m_unused40 = 0.1;
    m_expMax = 16.5;
    m_lightCurrent = 33060;
    m_alcMin = 33060;
    m_alcMax = 47514;
    m_gain = m_gainDefault = 1.0;
    m_deadZone = 15;
    m_deltK = -0.1f;
    m_deltBase = 1.8f;
    m_deltW2 = 1.0f;
    m_deltExpScale = 4.0f;
    m_pidBypassThresh = 0.25f;
    m_kp = m_ki = m_kd = 0.0f;
    memcpy(m_pidSets, kPidDefaults, sizeof(m_pidSets));
    memcpy(m_para, kPresetCameraAuto, sizeof(m_para));
    m_agcStatus = 1;
    m_lastCur = 170;
    m_lastTgt = 0;
    m_ratio1 = m_ratio2 = 0.0;
    memcpy(m_manuAlcTable, kManuAlcTable, sizeof(m_manuAlcTable));
    memset(m_alcRange, 0, sizeof(m_alcRange));
    m_alcValid = false;
}

K3ADimming *K3ADimming::Instance()
{
    // Реф.: ленивое `new` БЕЗ guard'а (не потокобезопасно). У нас — Мейерс.
    static K3ADimming inst;
    return &inst;
}

// --- элементарная математика ------------------------------------------------

double K3ADimming::CalLog(double x) { return 20.0 * std::log10(x); }
double K3ADimming::CalPow(double x) { return std::pow(10.0, x / 20.0); }

double K3ADimming::exposureRatioCal(double x)
{
    if (x >= 0.0)
        return x;
    const double r = m_ratio1;
    // Константы -0.8 (0xBFE999999999999A) и 1.8 (0x3FFCCCCCCCCCCCCD).
    return x * (-0.8 * r * r + 1.8 * r + 0.5);
}

double K3ADimming::CalDelt(const AUTO_DIMMING_PARA &p)
{
    return double(m_deltK) * std::pow(double(m_deltBase), double(m_deltExpScale) * p.d8)
             * CalLog(255.0 / double(p.target))
         - double(m_deltW2) * p.d10;
}

double K3ADimming::CalculatePidOut(double e)
{
    // ⚠️ История хранится во FLOAT, текущий член — в double. Округление
    // воспроизведено намеренно (важно для побитового совпадения).
    const float e1 = g_eLast;
    const float e2 = g_ePre;
    g_eLast = float(e);
    g_ePre = e1;
    return double(m_kp) * (e - double(e1))
         + double(m_ki) * e
         + double(m_kd) * (e - 2.0 * double(e1) + double(e2));
}

void K3ADimming::UpdataPid(double e)
{
    const double L = m_lgtTotal;
    const float *src;
    if (L >= m_para[0] && L <= m_para[1])
        src = (e > 0.0) ? m_pidSets[4] : m_pidSets[5];
    else if (L <= m_para[2])
        src = (e > 0.0) ? m_pidSets[0] : m_pidSets[1];
    else if (L > m_para[3])
        // ⚠️ МЁРТВАЯ ВЕТКА: тот же набор, что и выше — сравнение с p3 лишь
        // вырезает среднюю полосу.
        src = (e > 0.0) ? m_pidSets[0] : m_pidSets[1];
    else
        src = (e > 0.0) ? m_pidSets[2] : m_pidSets[3];
    m_kp = src[0];
    m_ki = src[1];
    m_kd = src[2];
}

double K3ADimming::UpdataLgtData(double lgt)
{
    const double hi = double(m_agcStatus ? m_agcOnHeadroom : m_agcOffHeadroom)
                    + m_para[4] + 6.0;
    if (m_agcStatus ? (hi < lgt) : (lgt > hi))
        return hi;
    return (lgt < m_para[0]) ? m_para[0] : lgt;
}

// --- свет / ток / регистры --------------------------------------------------

int K3ADimming::ldb2Lp(double dB)
{
    if (g_gateLampDisabled)
        return 0;
    // ⚠️ Реф.: разность приводится uint -> float -> double (потеря точности
    // воспроизведена намеренно).
    const double k = double(float(unsigned(m_alcMax - m_alcMin))) / kLampDivisor;
    const int w = int(std::pow(10.0, dB / 20.0) * 100.0);   // fcvtzs — усечение
    return int(double(m_alcMin) + k * double(w - 100));     // усечение
}

double K3ADimming::lp2LdB(int lp)
{
    if (g_gateLampDisabled)
        return 0.0;
    const double k = double(float(unsigned(m_alcMax - m_alcMin))) / kLampDivisor;
    const double B = double(unsigned(m_alcMin)) - k * 100.0;
    // ⚠️ Усечение ДО логарифма; домен не проверяется — при lp <= B будет
    // log10(<=0) → -inf/NaN, и это уйдёт прямо в m_lgtTotal.
    const int w = int((double(float(lp)) - B) / k);
    return 20.0 * std::log10(double(w) / 100.0);
}

double K3ADimming::Conver3ADimmingParaToLgt()
{
    return CalLog(m_exposureTime) + lp2LdB(m_lightCurrent) + CalLog(m_gain);
}

void K3ADimming::ConverLgtTo3ADimmingPara(double L)
{
    if (L >= m_para[0] && L <= m_para[1]) {
        // Весь бюджет — в экспозицию.
        m_alcLgt = 0.0;
        m_agcLgt = 0.0;
        m_aecLgt = L;
        m_exposureTime = CalPow(L);
        m_lightCurrent = m_alcMin;
        m_gain = m_gainDefault;
    } else if (L > m_para[2] && L <= m_para[3]) {
        // Экспозиция в насыщении, остаток — лампе.
        m_agcLgt = 0.0;
        m_exposureTime = m_expMax;
        m_aecLgt = m_para[1];
        m_alcLgt = L - m_para[2];
        m_lightCurrent = ldb2Lp(L - m_para[2]);
        m_gain = m_gainDefault;
    } else {
        // Остаток — усилению.
        m_exposureTime = m_expMax;
        m_lightCurrent = m_alcMax;
        m_aecLgt = m_para[1];
        m_alcLgt = m_para[3] - m_para[2];
        m_agcLgt = L - m_para[4];
        m_gain = CalPow(L - m_para[4]);
    }
}

unsigned K3ADimming::ExposureTimeToRegisterValue(double t)
{
    unsigned r;
    unsigned floorVal = 4;
    switch (m_sensorType) {
    case 1:
        r = unsigned(t * 40.0 * 1000.0 / 778.0);
        break;
    case 2:
        r = unsigned(2307.0 - (t * 72.0 * 1000.0 - 112.0) / 512.0);
        floorVal = 8;
        break;
    case 3:
        r = unsigned(t * 1000.0 * 8.0 * 16.0 / 576.0);
        break;
    case 4:
        // 0.04102 == 0x3FA50092CCF6BE38.
        r = unsigned(t / 0.04102);
        break;
    default:
        r = unsigned(t * 72.0 * 1000.0 * 16.0 / 1080.0);
        break;
    }
    return std::max(r, floorVal);
}

unsigned K3ADimming::gdbRegisterValue(double dB)
{
    switch (m_sensorType) {
    case 1:
        return unsigned(Float2FixedPointNumber(float(CalPow(dB)), 5, 8));
    case 2:
        return unsigned(1024.0 * (1.0 - std::pow(10.0, -dB / 20.0)));
    case 3:
        return std::min(unsigned(CalPow(dB) * 16.0), 63u);
    case 4:
        return std::min(unsigned(CalPow(dB) * 256.0), 992u);
    default:
        return unsigned(Float2FixedPointNumber(float(CalPow(dB)), 5, 7));
    }
}

// --- главный шаг ------------------------------------------------------------

void K3ADimming::Calculate3ADimmingPara(AUTO_DIMMING_PARA &p)
{
    m_lastCur = p.target;      // реф.: только для лога
    m_lastTgt = p.measured;
    m_ratio1 = p.d8;
    m_ratio2 = p.d10;

    const uint32_t diff = (p.target <= p.measured) ? (p.measured - p.target)
                                                   : (p.target - p.measured);
    if (unsigned(m_deadZone) > diff)
        return;                                    // мёртвая зона

    // ⚠️ КВИРК: пишется ОБРАТНО в структуру вызывающего.
    if (p.measured == 0)
        p.measured = 1;

    const double e = CalLog(double(p.target) / double(p.measured));

    double step;
    if (p.d8 <= double(m_pidBypassThresh)) {
        UpdataPid(e);
        step = exposureRatioCal(CalculatePidOut(e));
    } else if (e < 0.0) {
        step = CalDelt(p);
    } else {
        step = e;
    }

    m_lgtTotal = m_dimmingSpeed * step + Conver3ADimmingParaToLgt();
    m_lgtTotal = UpdataLgtData(m_lgtTotal);
    ConverLgtTo3ADimmingPara(m_lgtTotal);
}

void K3ADimming::GetDimmingResult(AUTO_DIMMING_RESULT &r)
{
    r.agc = int(gdbRegisterValue(m_agcLgt));
    r.alc = ldb2Lp(m_alcLgt);      // ⚠️ из m_alcLgt, а НЕ из m_lightCurrent
    r.aec = int(ExposureTimeToRegisterValue(m_exposureTime));
}

// --- настройка --------------------------------------------------------------

void K3ADimming::SetEndoSensorType(int t) { m_sensorType = t; }
void K3ADimming::SetDimmingSpeed(double v) { m_dimmingSpeed = v; }
void K3ADimming::SetLightCurrentRange(int cur) { m_lightCurrent = cur; }
void K3ADimming::SetAGCStatus(int st) { m_agcStatus = st; }
void K3ADimming::UpdateAlcMin(int v) { m_alcMin = v; }

void K3ADimming::SetAlcMax(double v)
{
    // ⚠️ КВИРК ОРИГИНАЛА: тело метода — ТОЛЬКО лог, значение НЕ СОХРАНЯЕТСЯ.
    LogPrintfEx(false, "[APP][D]: ", "value: %0.4f", v);
}

void K3ADimming::SetDimmingPidParam(const _KAutomaticDimmerParam &p)
{
    const float f[18] = { p.f00, p.f04, p.f08, p.f0c, p.f10, p.f14,
                          p.f18, p.f1c, p.f20, p.f24, p.f28, p.f2c,
                          p.f30, p.f34, p.f38, p.f3c, p.f40, p.f44 };
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 3; ++j)
            m_pidSets[i][j] = f[i * 3 + j];
    m_agcOnHeadroom  = int8_t(p.i48);
    m_agcOffHeadroom = int8_t(p.i49);
    m_deadZone = uint8_t(p.u4a);
    m_deltK    = p.f4c;
    m_deltBase = p.f50;
    // ⚠️ ПЕРЕКРЁСТНОЕ присваивание (как в реф.): 0x54 → m_deltExpScale(0x80),
    // 0x58 → m_deltW2(0x7c).
    m_deltExpScale = p.f54;
    m_deltW2       = p.f58;
}

void K3ADimming::SetCameraAutoDimmingParam()
{
    memcpy(m_para, kPresetCameraAuto, sizeof(m_para));
    m_expMax = CalPow(m_para[1]);
}

void K3ADimming::SetCameraManuDimmingLgtRange()
{
    memcpy(m_para, kPresetCameraManu, sizeof(m_para));
    m_expMax = CalPow(m_para[1]);
}

void K3ADimming::SetOAH0428DimmingParam()
{
    memcpy(m_para, kPresetOAH0428, sizeof(m_para));
    m_expMax = CalPow(m_para[1]);
}

void K3ADimming::SetOtherSensorDimmingParam()
{
    // ⚠️ Байт-клон SetCameraAutoDimmingParam, НО m_expMax НЕ ВЫСТАВЛЯЕТ.
    LogPrintfEx(false, "[APP][D]: ", "SetOtherSensorDimmingParam");
    memcpy(m_para, kPresetCameraAuto, sizeof(m_para));
}

void K3ADimming::SetOV6946DimmingParam()
{
    // Единственная работа с железом во всём классе.
    KPlControl pl;
    pl.WriteValueToPL(0xA0048010, 0x00350303);
    memcpy(m_para, kPresetOV6946, sizeof(m_para));
    m_expMax = CalPow(m_para[1]);
}

int K3ADimming::GetManuDimmingALC(int mode, int level)
{
    // ⚠️ Реф.: индексация плоская, БЕЗ проверки границ обоих индексов.
    const int *flat = &m_manuAlcTable[0][0];
    return flat[mode * 19 + level];
}
