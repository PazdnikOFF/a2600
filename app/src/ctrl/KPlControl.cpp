#include "ctrl/KPlControl.h"
#include "ctrl/KPlRegs.h"       // карта регистров PL (блоки/смещения вместо magic-адресов)
#include "alg/AlgParaManager.h"

#include <QDebug>

using namespace plreg;          // Tone::R, Awb::Gain, Ctrl::Status и т.д.

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

void KPlControl::SetGammaLut(const QVector<int> &lut)
{
    // Реф. SetGammaLut (дизасм X2000): значения (10 бит) пакуются парами
    // (v0&0x3ff) | ((v1&0x3ff)<<16) и пишутся в ТРИ банка Gamma::Bank0/1/2
    // (по 512 записей на 1024 значения). Затем защёлка Gamma::Ctrl |= LatchBit.
    // (На устройстве реф. ещё ждёт готовности — poll бита 2 в Gamma::Ctrl.)
    const addr_t bank[3] = { Gamma::Bank0, Gamma::Bank1, Gamma::Bank2 };
    const int pairs = lut.size() / 2;
    for (int i = 0; i < pairs; ++i) {
        const unsigned lo = static_cast<unsigned>(lut[2*i])     & 0x3ff;
        const unsigned hi = static_cast<unsigned>(lut[2*i + 1]) & 0x3ff;
        const unsigned v  = (hi << 16) | lo;
        const addr_t off = static_cast<addr_t>(i) * 4;
        for (int ch = 0; ch < 3; ++ch)
            WriteValueToPL(bank[ch] + off, v);
    }
    // Финализация: Gamma::Ctrl |= LatchBit (read-modify-write, защёлка LUT).
    unsigned int ctrl = 0;
    ReadValueFromPL(Gamma::Ctrl, ctrl);
    WriteValueToPL(Gamma::Ctrl, ctrl | Gamma::LatchBit);
}

void KPlControl::SetCCM0(int enable)
{
    // Реф. SetCCM0: Ccm0::Enable = enable (passthrough).
    WriteValueToPL(Ccm0::Enable, static_cast<unsigned int>(enable));
}

void KPlControl::SetCCM0Matrix(const int m[9])
{
    // Реф. SetCCM0Matrix (дизасм X2000): 4 пары коэффициентов упаковкой
    // m[2i]|(m[2i+1]<<16) → Ccm0::Matrix (0x04/08/0c/10); затем 9-й коэффициент
    // как 16-бит (ldurh) → Ccm0::Tail. Enable — отдельно (SetCCM0).
    addr_t reg = Ccm0::Matrix;
    for (int i = 0; i + 1 < 9; i += 2, reg += 4)
        WriteValueToPL(reg, static_cast<unsigned int>(m[i]) |
                            (static_cast<unsigned int>(m[i + 1]) << 16));
    WriteValueToPL(Ccm0::Tail, static_cast<unsigned int>(m[8]) & 0xffff);
}

// --- Параметры изображения → PL (карта регистров в ctrl/KPlRegs.h) ---

void KPlControl::SetColorEnhParam(bool enable, int level)
{
    // Реф.: ColorEnh::Enable = enable; значение уровня берётся из AlgParaManager
    // (массив из colenh_level.txt) → ColorEnh::Param.
    WriteValueToPL(ColorEnh::Enable, enable ? 1u : 0u);
    const int v = AlgParaManager::GetInstance().ColEnhLevelValue(level);
    WriteValueToPL(ColorEnh::Param, static_cast<unsigned int>(v));
}

void KPlControl::SetImageEnhValue(int level)
{
    // Реф.: значение уровня из AlgParaManager (ImgEnh/level_*.txt) → Image::EnhValue.
    const int v = AlgParaManager::GetInstance().ImgEnhLevelValue(level);
    WriteValueToPL(Image::EnhValue, static_cast<unsigned int>(v));
}

void KPlControl::SetColorR(int value)
{
    WriteValueToPL(Tone::R, static_cast<unsigned int>(value));
}

void KPlControl::SetColorB(int value)
{
    WriteValueToPL(Tone::B, static_cast<unsigned int>(value));
}

void KPlControl::SetColorC(int value)
{
    WriteValueToPL(Tone::C, static_cast<unsigned int>(value));
}

void KPlControl::SetToneValue(int r, int b, int c)
{
    // Реф. порядок записи: R, B, C.
    WriteValueToPL(Tone::R, static_cast<unsigned int>(r));
    WriteValueToPL(Tone::B, static_cast<unsigned int>(b));
    WriteValueToPL(Tone::C, static_cast<unsigned int>(c));
}

void KPlControl::SetBrightEQEnable(bool enable)
{
    WriteValueToPL(BrightEq::Enable, enable ? 1u : 0u);
}

void KPlControl::SetBrightEQLut(int level)
{
    AlgParaManager &alg = AlgParaManager::GetInstance();

    // Блок 1: гауссов фильтр → BrightEq::Gaussian.. (18 записей), каждая пакует
    // пару значений (реф.: младшее — 0x7fff, старшее — <<16, 15 бит).
    const QVector<int> &g = alg.BrightEqGaussian();
    for (int i = 0; i < 18; ++i) {
        const unsigned lo = (2*i     < g.size()) ? unsigned(g[2*i])     & 0x7fff : 0u;
        const unsigned hi = (2*i + 1 < g.size()) ? unsigned(g[2*i + 1]) & 0x7fff : 0u;
        WriteValueToPL(BrightEq::Gaussian + unsigned(i) * 4, (hi << 16) | lo);
    }

    // Блок 2: lumaGainLut уровня → BrightEq::LumaLut.. (512 записей, пары 12-бит).
    const int idx = qBound(1, level, 3) - 1;          // clamp(level,1,3)-1
    const QVector<int> &l = alg.BrightEqLumaLut(idx);
    for (int i = 0; i < 512; ++i) {
        const unsigned lo = (2*i     < l.size()) ? unsigned(l[2*i])     & 0xfff : 0u;
        const unsigned hi = (2*i + 1 < l.size()) ? unsigned(l[2*i + 1]) & 0xfff : 0u;
        WriteValueToPL(BrightEq::LumaLut + unsigned(i) * 4, (hi << 16) | lo);
    }
}

void KPlControl::SetAECAndAGCValue(unsigned int aec, unsigned int agc)
{
    // Реф.: w2 = (aec & 0xffff) | (agc << 16).
    const unsigned int v = (aec & 0xffff) | (agc << 16);
    WriteValueToPL(Front::AecAgc, v);
}

void KPlControl::SetAWBValue(unsigned int rGain, unsigned int bGain)
{
    // Реф.: w2 = (bGain & 0x1ffff) | (rGain << 16); строб Ctrl::AwbStrobe 1→0.
    const unsigned int v = (bGain & 0x1ffff) | (rGain << 16);
    WriteValueToPL(Awb::Gain, v);
    WriteValueToPL(Ctrl::AwbStrobe, 1);
    usleep(10);
    WriteValueToPL(Ctrl::AwbStrobe, 0);
}

void KPlControl::SetAwbCut(int low, int high)
{
    // Реф.: w2 = low | (high << 16).
    const unsigned int v = static_cast<unsigned int>(low)
                         | (static_cast<unsigned int>(high) << 16);
    WriteValueToPL(Awb::Cut, v);
}

void KPlControl::SetVistSwitch(bool enable)
{
    // Реф.: Vist::Switch = en; Awb::Start = !en (взаимоисключение с AWB-трактом).
    WriteValueToPL(Vist::Switch, enable ? 1u : 0u);
    WriteValueToPL(Awb::Start, enable ? 0u : 1u);
}

void KPlControl::SetVistMatrix(const unsigned int *data, int count)
{
    if (!data || count <= 0) return;
    // Реф.: пары значений → Vist::Matrix.. ; последний (нечётный) → Vist::Tail (16-бит).
    addr_t reg = Vist::Matrix;
    for (int i = 0; i + 1 < count; i += 2, reg += 4)
        WriteValueToPL(reg, data[i] | (data[i + 1] << 16));
    // хвостовой коэффициент
    WriteValueToPL(Vist::Tail, data[count - 1] & 0xffff);
}

void KPlControl::SetDenoiseLevel(int level)
{
    WriteValueToPL(Denoise::Level, static_cast<unsigned int>(level));
}

namespace {
// Записать 4 банка по count значений: база + i*4, банки со смещением Denoise::BankStep.
// Источник — плоский буфер src, окно банка = count (за пределами — 0).
void writeDenoiseBank(KPlControl *pl, unsigned long base, const int *src,
                      int srcCount, int count)
{
    for (int bank = 0; bank < 4; ++bank) {
        const unsigned long bbase = base + static_cast<unsigned long>(bank) * Denoise::BankStep;
        for (int i = 0; i < count; ++i) {
            const int idx = bank * count + i;
            const unsigned v = (idx < srcCount) ? unsigned(src[idx]) : 0u;
            pl->WriteValueToPL(bbase + static_cast<unsigned long>(i) * 4, v);
        }
    }
}
}

void KPlControl::SetGammaEnable(bool enable)
{
    WriteValueToPL(Gamma::Ctrl, enable ? 1u : 0u);
}

void KPlControl::SetZoomValue(unsigned int value)
{
    WriteValueToPL(Zoom::Value, value);
}

void KPlControl::SetCCM1(int enable)
{
    WriteValueToPL(Ccm1::Enable, static_cast<unsigned int>(enable));
}

void KPlControl::SetCCM1Matrix(const unsigned int *data, int count)
{
    // Реф. SetCCM1Matrix (дизасм X2000): пары data[2i]|(data[2i+1]<<16) → Ccm1::Matrix..;
    // затем хвост — последний коэффициент как 16-бит (ldurh) → Ccm1::Tail.
    if (!data || count <= 0) return;
    addr_t reg = Ccm1::Matrix;
    for (int i = 0; i + 1 < count; i += 2, reg += 4)
        WriteValueToPL(reg, data[i] | (data[i + 1] << 16));
    WriteValueToPL(Ccm1::Tail, data[count - 1] & 0xffff);
}

void KPlControl::SetChbStatus(int status)
{
    // Реф. SetChbStatus: status==0 → выкл (Chb::Enable=0); иначе → вкл (=1) +
    // запись CHb-значения (4-е из CHb-файла, реф. AlgParaManager+0x7a3c) в Chb::Value.
    if (status == 0) {
        WriteValueToPL(Chb::Enable, 0);
    } else {
        WriteValueToPL(Chb::Enable, 1);
        WriteValueToPL(Chb::Value,
                       static_cast<unsigned int>(AlgParaManager::GetInstance().ChbValue()));
    }
}

bool KPlControl::GetFpga1Version(unsigned int &version)
{
    version = 0;
    return ReadValueFromPL(Front::Fpga1Version, version);
}

bool KPlControl::GetFpga2Version(unsigned int &version)
{
    version = 0;
    return ReadValueFromPL(Ctrl::Version, version);
}

bool KPlControl::GetFpga3Version(unsigned int &version)
{
    version = 0;
    return ReadValueFromPL(Front::Fpga0Version, version);
}

bool KPlControl::GetFpga2System(unsigned int &value)
{
    value = 0;
    return ReadValueFromPL(Ctrl::Status, value);
}

bool KPlControl::ReadAWBValue(unsigned int &rGain, unsigned int &bGain)
{
    // Реф. ReadAWBValue (дизасм X2000, Awb::Value): распаковка — 14 бит на канал:
    //   bGain = v & 0x3fff (and w0,w4,#0x3fff); rGain = (v>>16) & 0x3fff (ubfx #16,#14).
    unsigned int v = 0;
    if (!ReadValueFromPL(Awb::Value, v))
        return false;
    rGain = (v >> 16) & 0x3fff;
    bGain = v & 0x3fff;
    return true;
}

namespace {
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
    WriteValueToPL(Video::FreezeStatus, static_cast<unsigned int>(status));
}

void KPlControl::SetVideoDisplay(int mode)
{
    WriteValueToPL(Display::Mode, static_cast<unsigned int>(mode));
}

void KPlControl::SetFreezeScalerIn(int a, int b)
{
    WriteValueToPL(FreezeScaler::In,
                   static_cast<unsigned int>(a) | (static_cast<unsigned int>(b) << 16));
}

void KPlControl::SetFreezeScalerOut(int a, int b)
{
    WriteValueToPL(FreezeScaler::Out,
                   static_cast<unsigned int>(a) | (static_cast<unsigned int>(b) << 16));
}

void KPlControl::SetFreezeScalerRatio(int a, int b)
{
    WriteValueToPL(FreezeScaler::Ratio,
                   static_cast<unsigned int>(a) | (static_cast<unsigned int>(b) << 16));
}

void KPlControl::SetCutPara(int a, int b)
{
    // Реф.: value = b | (a<<16).
    WriteValueToPL(Ccm0::CutPara,
                   static_cast<unsigned int>(b) | (static_cast<unsigned int>(a) << 16));
}

void KPlControl::SetFreezeVideoLoc(int a, int b, int c, int d)
{
    // Реф. SetFreezeVideoLoc: Video::FreezeLocA = a|(b<<16), FreezeLocB = c|(d<<16).
    WriteValueToPL(Video::FreezeLocA,
                   static_cast<unsigned int>(a) | (static_cast<unsigned int>(b) << 16));
    WriteValueToPL(Video::FreezeLocB,
                   static_cast<unsigned int>(c) | (static_cast<unsigned int>(d) << 16));
}

void KPlControl::SetLensSize(int a, int b)
{
    // Реф. SetLensSize: Lens::Size = a|(b<<16).
    WriteValueToPL(Lens::Size,
                   static_cast<unsigned int>(a) | (static_cast<unsigned int>(b) << 16));
}

void KPlControl::SetEnhanceSize(int, int)
{
    // Реф. SetEnhanceSize — пустая функция (только ret) в этой прошивке.
}

void KPlControl::SetDemoireEN(int value)
{
    // Реф. SetDemoireEN: Image::DemoireEn = value (passthrough).
    WriteValueToPL(Image::DemoireEn, static_cast<unsigned int>(value));
}

void KPlControl::SetContrastLevel(int)
{
    // Реф. SetContrastLevel — пустая функция (только ret) в этой прошивке.
}

void KPlControl::SetSensorRLut(const unsigned int *data, int count)
{
    writePairedLut(this, SensorLut::R, data, count);
}

void KPlControl::SetSensorGLut(const unsigned int *data, int count)
{
    writePairedLut(this, SensorLut::G, data, count);
}

void KPlControl::SetSensorBLut(const unsigned int *data, int count)
{
    writePairedLut(this, SensorLut::B, data, count);
}

void KPlControl::SetRbcLut(const unsigned int *hb, const unsigned int *hr,
                           const unsigned int *s, int count)
{
    // Реф.: три канала в соседние банки региона Rbc::Base, по одному слову на i.
    if (!hb || !hr || !s || count <= 0) return;
    for (int i = 0; i < count; ++i) {
        const addr_t off = static_cast<addr_t>(i) * 4;
        WriteValueToPL(Rbc::Hb + off, hb[i]);
        WriteValueToPL(Rbc::Hr + off, hr[i]);
        WriteValueToPL(Rbc::S  + off, s[i]);
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
        const addr_t off = static_cast<addr_t>(i) * 4;
        WriteValueToPL(Knee::Bank0 + off, v);
        WriteValueToPL(Knee::Bank1 + off, v);
        WriteValueToPL(Knee::Bank2 + off, v);
    }
    // Финализация: Knee::Ctrl |= LatchBit (read-modify-write, защёлка LUT).
    unsigned int ctrl = 0;
    ReadValueFromPL(Knee::Ctrl, ctrl);
    WriteValueToPL(Knee::Ctrl, ctrl | Knee::LatchBit);
}

void KPlControl::SetIrisTable(const int *data, int count, int shift)
{
    // Реф. SetIrisTable: 8 значений на регистр — (data[i]>>shift) в ниббл i*4.
    // Регион Iris::Table, count/8 записей (8040 → 1005).
    if (!data || count < 8) return;
    const int regs = count / 8;
    for (int r = 0; r < regs; ++r) {
        unsigned int v = 0;
        for (int k = 0; k < 8; ++k)
            v |= (static_cast<unsigned int>(data[r * 8 + k]) >> shift) << (k * 4);
        WriteValueToPL(Iris::Table + static_cast<addr_t>(r) * 4, v);
    }
}

void KPlControl::SetRealtimeVideoState(int state)
{
    WriteValueToPL(Display::RealtimeState, static_cast<unsigned int>(state));
}

void KPlControl::SetApmAreaDisplay(bool show)
{
    WriteValueToPL(Iris::ApmAreaShow, show ? 1u : 0u);
}

void KPlControl::VideoTest(int mode)
{
    // Реф.: значение сдвигается влево на 2 (mode<<2).
    WriteValueToPL(Front::VideoTest, static_cast<unsigned int>(mode) << 2);
}

void KPlControl::StartAWB()
{
    // Реф. StartAWB: взвести триггер AWB (Awb::Start) и снять VIST-переключатель.
    WriteValueToPL(Awb::Start, 1);
    WriteValueToPL(Vist::Switch, 0);
}

int KPlControl::ReadIrisValue()
{
    // Реф.: читаем Iris::Value, возвращаем младший байт.
    unsigned int v = 0;
    ReadValueFromPL(Iris::Value, v);
    return static_cast<int>(v & 0xff);
}

void KPlControl::SetCornerCutWay(int a, int b, int c)
{
    // Реф. SetCornerCutWay: AlgParaManager::SetCutCornerPara(a,b,c) считает LUT угла,
    // затем стрим kCutCornerLen(=1080) слов из выбранного банка (a) в CornerCut::Lut.
    auto &alg = AlgParaManager::GetInstance();
    alg.SetCutCornerPara(a, b, c);
    const QVector<int> &lut = alg.CutCornerLut(a);
    const int n = qMin(lut.size(), AlgParaManager::kCutCornerLen);
    for (int i = 0; i < n; ++i)
        WriteValueToPL(CornerCut::Lut + static_cast<addr_t>(i) * 4,
                       static_cast<unsigned int>(lut[i]));
}

void KPlControl::SetAuroraOffset(unsigned char a, unsigned char b)
{
    // Реф.: value = (a & 0xff) | ((b & 0xff) << 8).
    WriteValueToPL(Front::AuroraOffset, static_cast<unsigned int>(a) | (static_cast<unsigned int>(b) << 8));
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
    WriteValueToPL(Zoom::CaptureArea, v);
}

void KPlControl::SetVideoArea(int width, int height)
{
    // Реф.: передать размеры rect в AlgParaManager::resize (без прямой записи в PL).
    AlgParaManager::GetInstance().resize(width, height);
}

void KPlControl::SetCameraIrisType(int type, int subtype)
{
    // Реф. SetCameraIrisType: кодирование по type/subtype, флаг 0x30, → Iris::CameraType.
    unsigned int v;
    if (type == 2) {
        const unsigned int mode = (subtype == 5) ? 0x100u : 0x200u;
        v = (2u & 0x3) | mode | 0x30u;
    } else {
        const unsigned int hi = (type == 0) ? 0x500u : 0x400u;
        v = (static_cast<unsigned int>(type) & 0x3) | hi | 0x30u;
    }
    WriteValueToPL(Iris::CameraType, v);
}

void KPlControl::ReadBrightnessHistogramValue(unsigned short *out, int count)
{
    if (!out || count <= 0) return;
    // Реф. ReadBrightnessHistogramValue (дизасм): триггер=1 (Iris::HistTrigger), чтение
    // бинов (Iris::HistBins+, по 2 uint16 в 32-бит слове: low→out[2i], high→out[2i+1]),
    // затем сброс триггера=0.
    WriteValueToPL(Iris::HistTrigger, 1);
    const int words = count / 2;
    for (int i = 0; i < words; ++i) {
        unsigned int w = 0;
        ReadValueFromPL(Iris::HistBins + static_cast<addr_t>(i) * 4, w);
        out[2 * i]     = static_cast<unsigned short>(w & 0xffff);
        out[2 * i + 1] = static_cast<unsigned short>((w >> 16) & 0xffff);
    }
    WriteValueToPL(Iris::HistTrigger, 0);   // реф.: сброс триггера после чтения
}

void KPlControl::SetDenoiseLut(const DenoiseData &d)
{
    // Реф. SetDenoiseLut — карта регистров (ctrl/KPlRegs.h, namespace Denoise):
    //   заголовок dpc → Denoise::Dpc (4 банка +BankStep);
    //   kernelG → Denoise::KernelG (41×4 банка);
    //   kernelRB → Denoise::KernelRB (25×4);
    //   Lut → Denoise::Lut (256×4).
    // Банки в оригинале берут смежные окна одного буфера; здесь источник — плоские
    // массивы (kernelG 42 / kernelRB 25 / lut 256), лишние банки добиваются нулём.
    for (int i = 0; i < 4; ++i)
        WriteValueToPL(Denoise::Dpc + static_cast<addr_t>(i) * Denoise::BankStep,
                       static_cast<unsigned int>(d.dpc[i]));
    writeDenoiseBank(this, Denoise::KernelG,  d.kernelG,  d.kernelGCount,  41);
    writeDenoiseBank(this, Denoise::KernelRB, d.kernelRB, d.kernelRBCount, 25);
    writeDenoiseBank(this, Denoise::Lut,      d.lut,      d.lutCount,      256);
}
