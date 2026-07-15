#include "ctrl/KPlControl.h"
#include "alg/AlgParaManager.h"

#include <QDebug>

#if defined(__linux__)
#include <unistd.h>
#else
static inline void usleep(unsigned) {}   // десктоп: строб-паузы AWB — no-op
#endif

KPlControl::KPlControl(QObject *parent) : QObject(parent)
{
    // Открытие /dev/mem — лениво в KMemDevice при первой записи (реф. OPenDevice).
}

KPlControl::~KPlControl() = default;

bool KPlControl::WriteValueToPL(unsigned long physAddr, unsigned int value)
{
    // реф. WriteValueToPL → KMemDevice::WriteDevRegister (mmap+запись+read-back).
    if (traceOn_) {
        trace_.append({physAddr, value});
        qInfo("PL W 0x%08lx = 0x%08x", physAddr, value);
    }
    const int r = mem_.WriteDevRegister(static_cast<unsigned int>(physAddr), value);
    return r == KMemDevice::Ok ? true : traceOn_; // на десктопе (нет /dev/mem) — успех при трассировке
}

bool KPlControl::ReadValueFromPL(unsigned long physAddr, unsigned int &value)
{
    // реф. ReadValueFromPL → KMemDevice::ReadDevRegister.
    return mem_.ReadDevRegister(static_cast<unsigned int>(physAddr), value) == KMemDevice::Ok;
}

// Базовые адреса PL из дизассемблера оригинала.
namespace {
constexpr unsigned long kGammaBase = 0xa1830000;   // SetGammaLut (ctrl/latch)
// Реф. SetGammaLut: три банка LUT со сдвигом 0x800 от 0xa1830800.
constexpr unsigned long kGammaCh[3] = {0x800, 0x1000, 0x1800};
constexpr unsigned long kCcm0Base  = 0xa1860000;   // SetCCM0 enable
constexpr unsigned long kCcm0Tail  = 0xa1860014;   // 9-й коэффициент (хвост), реф.
}

void KPlControl::SetGammaLut(const QVector<int> &lut)
{
    // Реф. SetGammaLut (дизасм X2000): значения (10 бит) пакуются парами
    // (v0&0x3ff) | ((v1&0x3ff)<<16) и пишутся в ТРИ банка 0xa1830800/1000/1800
    // (по 512 записей на 1024 значения). Затем защёлка 0xa1830000 |= 0x2.
    // (На устройстве реф. ещё ждёт готовности — poll бита 2 в 0xa1830000.)
    const int pairs = lut.size() / 2;
    for (int i = 0; i < pairs; ++i) {
        const unsigned lo = static_cast<unsigned>(lut[2*i])     & 0x3ff;
        const unsigned hi = static_cast<unsigned>(lut[2*i + 1]) & 0x3ff;
        const unsigned v  = (hi << 16) | lo;
        const unsigned long off = static_cast<unsigned long>(i) * 4;
        for (int ch = 0; ch < 3; ++ch)
            WriteValueToPL(kGammaBase + kGammaCh[ch] + off, v);
    }
    // Финализация: 0xa1830000 |= 0x2 (реф. read-modify-write, защёлка LUT).
    unsigned int ctrl = 0;
    ReadValueFromPL(kGammaBase, ctrl);
    WriteValueToPL(kGammaBase, ctrl | 0x2);
}

void KPlControl::SetCCM0(int enable)
{
    // Реф. SetCCM0: 0xa1860000 = enable (passthrough).
    WriteValueToPL(kCcm0Base, static_cast<unsigned int>(enable));
}

void KPlControl::SetCCM0Matrix(const int m[9])
{
    // Реф. SetCCM0Matrix (дизасм X2000): 4 пары коэффициентов упаковкой
    // m[2i]|(m[2i+1]<<16) → 0xa1860004,08,0c,10; затем 9-й коэффициент как
    // 16-бит (ldurh) → фиксированный 0xa1860014 (хвост). Enable — отдельно (SetCCM0).
    unsigned long reg = kCcm0Base + 0x4;
    for (int i = 0; i + 1 < 9; i += 2, reg += 4)
        WriteValueToPL(reg, static_cast<unsigned int>(m[i]) |
                            (static_cast<unsigned int>(m[i + 1]) << 16));
    WriteValueToPL(kCcm0Tail, static_cast<unsigned int>(m[8]) & 0xffff);
}

// --- Параметры изображения → PL (карты регистров из дизассемблера X2000) ---

namespace {
constexpr unsigned long kColorEnhEnable = 0xa18f0008;  // SetColorEnhParam enable
constexpr unsigned long kColorEnhParam  = 0xa18f0024;  // SetColorEnhParam значение
constexpr unsigned long kImageEnhParam  = 0xa1850058;  // SetImageEnhValue
constexpr unsigned long kToneBase       = 0xa1870000;  // C(+0)/R(+4)/B(+8)
constexpr unsigned long kBrightEQEnable = 0xa1950000;  // SetBrightEQEnalbe
constexpr unsigned long kAecAgcReg      = 0xa0048020;  // SetAECAndAGCValue
constexpr unsigned long kAwbGainReg     = 0xa184000c;  // SetAWBValue гейны
constexpr unsigned long kAwbTrigReg     = 0xa100019c;  // SetAWBValue строб
constexpr unsigned long kAwbCutReg      = 0xa1840018;  // SetAwbCut
}

void KPlControl::SetColorEnhParam(bool enable, int level)
{
    // Реф.: WriteValueToPL(0xa18f0008, enable); значение уровня берётся из
    // AlgParaManager (массив, загруженный из colenh_level.txt) → 0xa18f0024.
    WriteValueToPL(kColorEnhEnable, enable ? 1u : 0u);
    const int v = AlgParaManager::GetInstance().ColEnhLevelValue(level);
    WriteValueToPL(kColorEnhParam, static_cast<unsigned int>(v));
}

void KPlControl::SetImageEnhValue(int level)
{
    // Реф.: значение уровня из AlgParaManager (ImgEnh/level_*.txt) → 0xa1850058.
    const int v = AlgParaManager::GetInstance().ImgEnhLevelValue(level);
    WriteValueToPL(kImageEnhParam, static_cast<unsigned int>(v));
}

void KPlControl::SetColorR(int value)
{
    WriteValueToPL(kToneBase + 0x4, static_cast<unsigned int>(value));
}

void KPlControl::SetColorB(int value)
{
    WriteValueToPL(kToneBase + 0x8, static_cast<unsigned int>(value));
}

void KPlControl::SetColorC(int value)
{
    WriteValueToPL(kToneBase + 0x0, static_cast<unsigned int>(value));
}

void KPlControl::SetToneValue(int r, int b, int c)
{
    // Реф. порядок записи: +0x4 (R), +0x8 (B), +0x0 (C).
    WriteValueToPL(kToneBase + 0x4, static_cast<unsigned int>(r));
    WriteValueToPL(kToneBase + 0x8, static_cast<unsigned int>(b));
    WriteValueToPL(kToneBase + 0x0, static_cast<unsigned int>(c));
}

void KPlControl::SetBrightEQEnable(bool enable)
{
    WriteValueToPL(kBrightEQEnable, enable ? 1u : 0u);
}

void KPlControl::SetBrightEQLut(int level)
{
    AlgParaManager &alg = AlgParaManager::GetInstance();

    // Блок 1: гауссов фильтр → 0xa1950004..0x004c. 18 записей, каждая пакует
    // пару значений (реф.: младшее — 0x7fff, старшее — <<16, 15 бит).
    const QVector<int> &g = alg.BrightEqGaussian();
    for (int i = 0; i < 18; ++i) {
        const unsigned lo = (2*i     < g.size()) ? unsigned(g[2*i])     & 0x7fff : 0u;
        const unsigned hi = (2*i + 1 < g.size()) ? unsigned(g[2*i + 1]) & 0x7fff : 0u;
        WriteValueToPL(0xa1950004 + unsigned(i) * 4, (hi << 16) | lo);
    }

    // Блок 2: lumaGainLut уровня → 0xa1958000..0x8800. 512 записей, пары 12-бит.
    const int idx = qBound(1, level, 3) - 1;          // clamp(level,1,3)-1
    const QVector<int> &l = alg.BrightEqLumaLut(idx);
    for (int i = 0; i < 512; ++i) {
        const unsigned lo = (2*i     < l.size()) ? unsigned(l[2*i])     & 0xfff : 0u;
        const unsigned hi = (2*i + 1 < l.size()) ? unsigned(l[2*i + 1]) & 0xfff : 0u;
        WriteValueToPL(0xa1958000 + unsigned(i) * 4, (hi << 16) | lo);
    }
}

void KPlControl::SetAECAndAGCValue(unsigned int aec, unsigned int agc)
{
    // Реф.: w2 = (aec & 0xffff) | (agc << 16).
    const unsigned int v = (aec & 0xffff) | (agc << 16);
    WriteValueToPL(kAecAgcReg, v);
}

void KPlControl::SetAWBValue(unsigned int rGain, unsigned int bGain)
{
    // Реф.: w2 = (bGain & 0x1ffff) | (rGain << 16); строб 0xa100019c 1→0.
    const unsigned int v = (bGain & 0x1ffff) | (rGain << 16);
    WriteValueToPL(kAwbGainReg, v);
    WriteValueToPL(kAwbTrigReg, 1);
    usleep(10);
    WriteValueToPL(kAwbTrigReg, 0);
}

void KPlControl::SetAwbCut(int low, int high)
{
    // Реф.: w2 = low | (high << 16).
    const unsigned int v = static_cast<unsigned int>(low)
                         | (static_cast<unsigned int>(high) << 16);
    WriteValueToPL(kAwbCutReg, v);
}

namespace {
constexpr unsigned long kVistBase    = 0xa18e0000;  // SetVistSwitch/Matrix
constexpr unsigned long kVistAwbInv  = 0xa1840000;  // инверсный флаг в AWB-регионе
constexpr unsigned long kDenoiseLvl  = 0xa1940008;  // SetDenoiseLevel
constexpr unsigned long kDenoiseBase = 0xa1940000;  // регион SetDenoiseLut
}

void KPlControl::SetVistSwitch(bool enable)
{
    // Реф.: 0xa18e0000 = en; 0xa1840000 = !en (взаимоисключение с AWB-трактом).
    WriteValueToPL(kVistBase, enable ? 1u : 0u);
    WriteValueToPL(kVistAwbInv, enable ? 0u : 1u);
}

void KPlControl::SetVistMatrix(const unsigned int *data, int count)
{
    if (!data || count <= 0) return;
    // Реф.: пары значений → 0xa18e0004.. ; последний (нечётный) → 0xa18e0014 (16-бит).
    int i = 0;
    unsigned long reg = kVistBase + 0x4;
    for (; i + 1 < count; i += 2, reg += 4)
        WriteValueToPL(reg, data[i] | (data[i + 1] << 16));
    // хвостовой коэффициент
    WriteValueToPL(kVistBase + 0x14, data[count - 1] & 0xffff);
}

void KPlControl::SetDenoiseLevel(int level)
{
    WriteValueToPL(kDenoiseLvl, static_cast<unsigned int>(level));
}

namespace {
// Записать 4 банка по count значений: база + i*4, банки со смещением +0x1000.
// Источник — плоский буфер src, окно банка = count (за пределами — 0).
void writeDenoiseBank(KPlControl *pl, unsigned long base, const int *src,
                      int srcCount, int count)
{
    for (int bank = 0; bank < 4; ++bank) {
        const unsigned long bbase = base + static_cast<unsigned long>(bank) * 0x1000;
        for (int i = 0; i < count; ++i) {
            const int idx = bank * count + i;
            const unsigned v = (idx < srcCount) ? unsigned(src[idx]) : 0u;
            pl->WriteValueToPL(bbase + static_cast<unsigned long>(i) * 4, v);
        }
    }
}
}

namespace {
constexpr unsigned long kGammaEnableReg = 0xa1830000;  // SetGammaEnable
constexpr unsigned long kZoomReg        = 0xa18d0004;   // SetZoomValue
constexpr unsigned long kCcm1Base       = 0xa1880000;   // SetCCM1/Matrix
constexpr unsigned long kChbEnableReg   = 0xa1900008;   // SetChbStatus enable-строб
constexpr unsigned long kChbValueReg    = 0xa1900018;   // SetChbStatus значение
constexpr unsigned long kFpga1VersionReg = 0xa004a044;  // GetFpga1Version
}

void KPlControl::SetGammaEnable(bool enable)
{
    WriteValueToPL(kGammaEnableReg, enable ? 1u : 0u);
}

void KPlControl::SetZoomValue(unsigned int value)
{
    WriteValueToPL(kZoomReg, value);
}

void KPlControl::SetCCM1(int enable)
{
    WriteValueToPL(kCcm1Base, static_cast<unsigned int>(enable));
}

void KPlControl::SetCCM1Matrix(const unsigned int *data, int count)
{
    // Реф. SetCCM1Matrix (дизасм X2000): пары data[2i]|(data[2i+1]<<16) → 0xa1880004..;
    // затем хвост — последний коэффициент как 16-бит (ldurh) → фиксированный 0xa1880014.
    if (!data || count <= 0) return;
    unsigned long reg = kCcm1Base + 0x4;
    for (int i = 0; i + 1 < count; i += 2, reg += 4)
        WriteValueToPL(reg, data[i] | (data[i + 1] << 16));
    WriteValueToPL(kCcm1Base + 0x14, data[count - 1] & 0xffff);
}

void KPlControl::SetChbStatus(int status)
{
    // Реф. SetChbStatus: status==0 → выкл (0xa1900008=0); иначе → вкл (=1) +
    // запись CHb-значения (4-е из CHb-файла, реф. AlgParaManager+0x7a3c) в 0xa1900018.
    if (status == 0) {
        WriteValueToPL(kChbEnableReg, 0);
    } else {
        WriteValueToPL(kChbEnableReg, 1);
        WriteValueToPL(kChbValueReg,
                       static_cast<unsigned int>(AlgParaManager::GetInstance().ChbValue()));
    }
}

bool KPlControl::GetFpga1Version(unsigned int &version)
{
    version = 0;
    return ReadValueFromPL(kFpga1VersionReg, version);
}

bool KPlControl::GetFpga2Version(unsigned int &version)
{
    version = 0;
    return ReadValueFromPL(0xa1000000, version);
}

bool KPlControl::GetFpga3Version(unsigned int &version)
{
    version = 0;
    return ReadValueFromPL(0xa0060000, version);
}

bool KPlControl::GetFpga2System(unsigned int &value)
{
    value = 0;
    return ReadValueFromPL(0xa1000008, value);
}

bool KPlControl::ReadAWBValue(unsigned int &rGain, unsigned int &bGain)
{
    // Реф. ReadAWBValue (дизасм X2000, 0xa1840014): распаковка — 14 бит на канал:
    //   bGain = v & 0x3fff (and w0,w4,#0x3fff); rGain = (v>>16) & 0x3fff (ubfx #16,#14).
    unsigned int v = 0;
    if (!ReadValueFromPL(0xa1840014, v))
        return false;
    rGain = (v >> 16) & 0x3fff;
    bGain = v & 0x3fff;
    return true;
}

namespace {
constexpr unsigned long kFreezeStatusReg  = 0xa180002c;  // SetFreezeStatus
constexpr unsigned long kVideoDisplayReg  = 0xa0080028;  // SetVideoDisplay
constexpr unsigned long kFreezeScalerBase = 0xa1910000;  // In+0xc/Out+0x10/Ratio+0x8
constexpr unsigned long kCutParaReg       = 0xa1860018;  // SetCutPara
constexpr unsigned long kSensorLutR       = 0xa1820800;  // SetSensorRLut
constexpr unsigned long kSensorLutG       = 0xa1821000;  // SetSensorGLut
constexpr unsigned long kSensorLutB       = 0xa1821800;  // SetSensorBLut

// Записать пары значений в LUT: base+i*4 = data[2i]|(data[2i+1]<<16).
void writePairedLut(KPlControl *pl, unsigned long base, const unsigned int *data, int count)
{
    if (!data || count <= 0) return;
    unsigned long reg = base;
    for (int i = 0; i + 1 < count; i += 2, reg += 4)
        pl->WriteValueToPL(reg, data[i] | (data[i + 1] << 16));
}
}

void KPlControl::SetFreezeStatus(int status)
{
    WriteValueToPL(kFreezeStatusReg, static_cast<unsigned int>(status));
}

void KPlControl::SetVideoDisplay(int mode)
{
    WriteValueToPL(kVideoDisplayReg, static_cast<unsigned int>(mode));
}

void KPlControl::SetFreezeScalerIn(int a, int b)
{
    WriteValueToPL(kFreezeScalerBase + 0xc,
                   static_cast<unsigned int>(a) | (static_cast<unsigned int>(b) << 16));
}

void KPlControl::SetFreezeScalerOut(int a, int b)
{
    WriteValueToPL(kFreezeScalerBase + 0x10,
                   static_cast<unsigned int>(a) | (static_cast<unsigned int>(b) << 16));
}

void KPlControl::SetFreezeScalerRatio(int a, int b)
{
    WriteValueToPL(kFreezeScalerBase + 0x8,
                   static_cast<unsigned int>(a) | (static_cast<unsigned int>(b) << 16));
}

void KPlControl::SetCutPara(int a, int b)
{
    // Реф.: value = b | (a<<16).
    WriteValueToPL(kCutParaReg,
                   static_cast<unsigned int>(b) | (static_cast<unsigned int>(a) << 16));
}

void KPlControl::SetFreezeVideoLoc(int a, int b, int c, int d)
{
    // Реф. SetFreezeVideoLoc: 0xa1800024 = a|(b<<16), 0xa1800028 = c|(d<<16).
    WriteValueToPL(0xa1800024,
                   static_cast<unsigned int>(a) | (static_cast<unsigned int>(b) << 16));
    WriteValueToPL(0xa1800028,
                   static_cast<unsigned int>(c) | (static_cast<unsigned int>(d) << 16));
}

void KPlControl::SetLensSize(int a, int b)
{
    // Реф. SetLensSize: 0xa189000c = a|(b<<16).
    WriteValueToPL(0xa189000c,
                   static_cast<unsigned int>(a) | (static_cast<unsigned int>(b) << 16));
}

void KPlControl::SetEnhanceSize(int, int)
{
    // Реф. SetEnhanceSize — пустая функция (только ret) в этой прошивке.
}

void KPlControl::SetDemoireEN(int value)
{
    // Реф. SetDemoireEN: 0xa18501cc = value (passthrough).
    WriteValueToPL(0xa18501cc, static_cast<unsigned int>(value));
}

void KPlControl::SetContrastLevel(int)
{
    // Реф. SetContrastLevel — пустая функция (только ret) в этой прошивке.
}

void KPlControl::SetSensorRLut(const unsigned int *data, int count)
{
    writePairedLut(this, kSensorLutR, data, count);
}

void KPlControl::SetSensorGLut(const unsigned int *data, int count)
{
    writePairedLut(this, kSensorLutG, data, count);
}

void KPlControl::SetSensorBLut(const unsigned int *data, int count)
{
    writePairedLut(this, kSensorLutB, data, count);
}

void KPlControl::SetRbcLut(const unsigned int *hb, const unsigned int *hr,
                           const unsigned int *s, int count)
{
    // Реф.: три канала в соседние банки региона 0xa1878000, по одному слову на i.
    if (!hb || !hr || !s || count <= 0) return;
    for (int i = 0; i < count; ++i) {
        const unsigned long off = static_cast<unsigned long>(i) * 4;
        WriteValueToPL(0xa1878200 + off, hb[i]);   // Hb
        WriteValueToPL(0xa1878100 + off, hr[i]);   // Hr
        WriteValueToPL(0xa1878000 + off, s[i]);    // S
    }
}

void KPlControl::SetKneeLut(const int *data, int count)
{
    // Реф. SetKneeLut: значения (10 бит) парами → 3 банка; затем защёлка бит1.
    // Реф. ограничивает число значений сверху 1024 (min(count,0x400)).
    if (!data || count <= 0) return;
    if (count > 1024) count = 1024;
    const int pairs = count / 2;
    for (int i = 0; i < pairs; ++i) {
        const unsigned lo = static_cast<unsigned>(data[2*i])     & 0x3ff;
        const unsigned hi = static_cast<unsigned>(data[2*i + 1]) & 0x3ff;
        const unsigned v  = (hi << 16) | lo;
        const unsigned long off = static_cast<unsigned long>(i) * 4;
        WriteValueToPL(0xa1930800 + off, v);
        WriteValueToPL(0xa1931000 + off, v);
        WriteValueToPL(0xa1931800 + off, v);
    }
    // Финализация: 0xa1930000 |= 0x2 (реф. read-modify-write, защёлка LUT).
    unsigned int ctrl = 0;
    ReadValueFromPL(0xa1930000, ctrl);
    WriteValueToPL(0xa1930000, ctrl | 0x2);
}

void KPlControl::SetIrisTable(const int *data, int count, int shift)
{
    // Реф. SetIrisTable: 8 значений на регистр — (data[i]>>shift) в ниббл i*4.
    // Регион 0xa18a8000, count/8 записей (8040 → 1005).
    if (!data || count < 8) return;
    const int regs = count / 8;
    for (int r = 0; r < regs; ++r) {
        unsigned int v = 0;
        for (int k = 0; k < 8; ++k)
            v |= (static_cast<unsigned int>(data[r * 8 + k]) >> shift) << (k * 4);
        WriteValueToPL(0xa18a8000 + static_cast<unsigned long>(r) * 4, v);
    }
}

void KPlControl::SetRealtimeVideoState(int state)
{
    WriteValueToPL(0xa0080024, static_cast<unsigned int>(state));
}

void KPlControl::SetApmAreaDisplay(bool show)
{
    WriteValueToPL(0xa18a0008, show ? 1u : 0u);
}

void KPlControl::VideoTest(int mode)
{
    // Реф.: значение сдвигается влево на 2 (mode<<2).
    WriteValueToPL(0xa004a040, static_cast<unsigned int>(mode) << 2);
}

void KPlControl::StartAWB()
{
    // Реф. StartAWB: взвести триггер AWB и снять VIST-переключатель.
    WriteValueToPL(0xa1840000, 1);
    WriteValueToPL(0xa18e0000, 0);
}

int KPlControl::ReadIrisValue()
{
    // Реф.: читаем 0xa18a0004, возвращаем младший байт.
    unsigned int v = 0;
    ReadValueFromPL(0xa18a0004, v);
    return static_cast<int>(v & 0xff);
}

void KPlControl::SetAuroraOffset(unsigned char a, unsigned char b)
{
    // Реф.: value = (a & 0xff) | ((b & 0xff) << 8).
    WriteValueToPL(0xa004a02c, static_cast<unsigned int>(a) | (static_cast<unsigned int>(b) << 8));
}

namespace {
// Знак-величина точки для SetVideoCaptureArea (реф.): ((|v|*2)|(v<0?0x100:0))&0x1ff.
unsigned int encodeSignMag(int v)
{
    unsigned int e = static_cast<unsigned int>(v < 0 ? -v : v) * 2;
    if (v < 0) e |= 0x100;
    return e & 0x1ff;
}
}

void KPlControl::SetVideoCaptureArea(int x, int y)
{
    const unsigned int v = encodeSignMag(x) | (encodeSignMag(y) << 16);
    WriteValueToPL(0xa18d0008, v);
}

void KPlControl::SetVideoArea(int width, int height)
{
    // Реф.: передать размеры rect в AlgParaManager::resize (без прямой записи в PL).
    AlgParaManager::GetInstance().resize(width, height);
}

void KPlControl::SetCameraIrisType(int type, int subtype)
{
    // Реф. SetCameraIrisType: кодирование по type/subtype, флаг 0x30, → 0xa18a0000.
    unsigned int v;
    if (type == 2) {
        const unsigned int mode = (subtype == 5) ? 0x100u : 0x200u;
        v = (2u & 0x3) | mode | 0x30u;
    } else {
        const unsigned int hi = (type == 0) ? 0x500u : 0x400u;
        v = (static_cast<unsigned int>(type) & 0x3) | hi | 0x30u;
    }
    WriteValueToPL(0xa18a0000, v);
}

void KPlControl::ReadBrightnessHistogramValue(unsigned short *out, int count)
{
    if (!out || count <= 0) return;
    // Реф. ReadBrightnessHistogramValue (дизасм): триггер=1 (0xa18a0010), чтение бинов
    // (0xa18a9000+, по 2 uint16 в 32-бит слове: low→out[2i], high→out[2i+1]), затем
    // сброс триггера=0.
    WriteValueToPL(0xa18a0010, 1);
    const int words = count / 2;
    for (int i = 0; i < words; ++i) {
        unsigned int w = 0;
        ReadValueFromPL(0xa18a9000 + static_cast<unsigned long>(i) * 4, w);
        out[2 * i]     = static_cast<unsigned short>(w & 0xffff);
        out[2 * i + 1] = static_cast<unsigned short>((w >> 16) & 0xffff);
    }
    WriteValueToPL(0xa18a0010, 0);   // реф.: сброс триггера после чтения
}

void KPlControl::SetDenoiseLut(const DenoiseData &d)
{
    // Реф. SetDenoiseLut — карта регистров из дизассемблера:
    //   заголовок dpc → 0xa1941010/2010/3010/4010 (4 значения);
    //   kernelG → 0xa1941600 (41×4 банка, +0x1000);
    //   kernelRB → 0xa1941500 (25×4);
    //   Lut → 0xa1941100 (256×4).
    // Банки в оригинале берут смежные окна одного буфера; здесь источник — плоские
    // массивы (kernelG 42 / kernelRB 25 / lut 256), лишние банки добиваются нулём.
    for (int i = 0; i < 4; ++i)
        WriteValueToPL(kDenoiseBase + 0x1010 + static_cast<unsigned long>(i) * 0x1000,
                       static_cast<unsigned int>(d.dpc[i]));
    writeDenoiseBank(this, kDenoiseBase + 0x1600, d.kernelG,  d.kernelGCount,  41);
    writeDenoiseBank(this, kDenoiseBase + 0x1500, d.kernelRB, d.kernelRBCount, 25);
    writeDenoiseBank(this, kDenoiseBase + 0x1100, d.lut,      d.lutCount,      256);
}
