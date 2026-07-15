#include "ctrl/KPlControl.h"
#include "ctrl/KPlRegs.h"       // карта регистров PL (REG_* макросы вместо magic-адресов)
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

void KPlControl::SetGammaLut()
{
    // Реф. SetGammaLut (дизасм X2000): void — читает LUT из AlgParaManager (массив,
    // byte 0x1514 / word 0x545, заполнен CalGammaLut). Значения (10 бит) пакуются
    // парами (v0&0x3ff) | ((v1&0x3ff)<<16) → 3 банка REG_GAMMA_BANK0/1/2 (512 записей
    // на 1024 значения). Затем защёлка REG_GAMMA_CTRL |= LatchBit. (На устройстве реф.
    // ещё ждёт готовности — poll бита 2 в REG_GAMMA_CTRL.)
    const QVector<int> &lut = AlgParaManager::GetInstance().CurGammaLut();
    const unsigned long bank[3] = { REG_GAMMA_BANK0, REG_GAMMA_BANK1, REG_GAMMA_BANK2 };
    const int pairs = lut.size() / 2;
    for (int i = 0; i < pairs; ++i) {
        const unsigned lo = static_cast<unsigned>(lut[2*i])     & 0x3ff;
        const unsigned hi = static_cast<unsigned>(lut[2*i + 1]) & 0x3ff;
        const unsigned v  = (hi << 16) | lo;
        const unsigned long off = static_cast<unsigned long>(i) * 4;
        for (int ch = 0; ch < 3; ++ch)
            WriteValueToPL(bank[ch] + off, v);
    }
    // Финализация: REG_GAMMA_CTRL |= LatchBit (read-modify-write, защёлка LUT).
    unsigned int ctrl = 0;
    ReadValueFromPL(REG_GAMMA_CTRL, ctrl);
    WriteValueToPL(REG_GAMMA_CTRL, ctrl | GAMMA_LATCH_BIT);
}

void KPlControl::SetCCM0(int enable)
{
    // Реф. SetCCM0: REG_CCM0_ENABLE = enable (passthrough).
    WriteValueToPL(REG_CCM0_ENABLE, static_cast<unsigned int>(enable));
}

void KPlControl::SetCCM0Matrix(const unsigned int *data, int count)
{
    // Реф. SetCCM0Matrix(uint*, int) (дизасм X2000): пары data[2i]|(data[2i+1]<<16)
    // → REG_CCM0_MATRIX (0x04/08/0c/10); затем последний коэффициент как 16-бит
    // (ldurh) → REG_CCM0_TAIL. Enable — отдельно (SetCCM0).
    if (!data || count <= 0) return;
    unsigned long reg = REG_CCM0_MATRIX;
    for (int i = 0; i + 1 < count; i += 2, reg += 4)
        WriteValueToPL(reg, data[i] | (data[i + 1] << 16));
    WriteValueToPL(REG_CCM0_TAIL, data[count - 1] & 0xffff);
}

// --- Параметры изображения → PL (карта регистров в ctrl/KPlRegs.h) ---

void KPlControl::SetColorEnhParam(bool enable, int level)
{
    // Реф.: REG_COLOR_ENH_ENABLE = enable; значение уровня берётся из AlgParaManager
    // (массив из colenh_level.txt) → REG_COLOR_ENH_PARAM.
    WriteValueToPL(REG_COLOR_ENH_ENABLE, enable ? 1u : 0u);
    const int v = AlgParaManager::GetInstance().ColEnhLevelValue(level);
    WriteValueToPL(REG_COLOR_ENH_PARAM, static_cast<unsigned int>(v));
}

void KPlControl::SetImageEnhValue(int level)
{
    // Реф.: значение уровня из AlgParaManager (ImgEnh/level_*.txt) → REG_IMAGE_ENH.
    const int v = AlgParaManager::GetInstance().ImgEnhLevelValue(level);
    WriteValueToPL(REG_IMAGE_ENH, static_cast<unsigned int>(v));
}

void KPlControl::SetColorR(int value)
{
    WriteValueToPL(REG_TONE_R, static_cast<unsigned int>(value));
}

void KPlControl::SetColorB(int value)
{
    WriteValueToPL(REG_TONE_B, static_cast<unsigned int>(value));
}

void KPlControl::SetColorC(int value)
{
    WriteValueToPL(REG_TONE_C, static_cast<unsigned int>(value));
}

void KPlControl::SetToneValue(int r, int b, int c)
{
    // Реф. порядок записи: R, B, C.
    WriteValueToPL(REG_TONE_R, static_cast<unsigned int>(r));
    WriteValueToPL(REG_TONE_B, static_cast<unsigned int>(b));
    WriteValueToPL(REG_TONE_C, static_cast<unsigned int>(c));
}

void KPlControl::SetBrightEQEnable(bool enable)
{
    WriteValueToPL(REG_BRIGHT_EQ_ENABLE, enable ? 1u : 0u);
}

void KPlControl::SetBrightEQLut(int level)
{
    AlgParaManager &alg = AlgParaManager::GetInstance();

    // Блок 1: гауссов фильтр → REG_BRIGHT_EQ_GAUSSIAN.. (18 записей), каждая пакует
    // пару значений (реф.: младшее — 0x7fff, старшее — <<16, 15 бит).
    const QVector<int> &g = alg.BrightEqGaussian();
    for (int i = 0; i < 18; ++i) {
        const unsigned lo = (2*i     < g.size()) ? unsigned(g[2*i])     & 0x7fff : 0u;
        const unsigned hi = (2*i + 1 < g.size()) ? unsigned(g[2*i + 1]) & 0x7fff : 0u;
        WriteValueToPL(REG_BRIGHT_EQ_GAUSSIAN + unsigned(i) * 4, (hi << 16) | lo);
    }

    // Блок 2: lumaGainLut уровня → REG_BRIGHT_EQ_LUMA_LUT.. (512 записей, пары 12-бит).
    const int idx = qBound(1, level, 3) - 1;          // clamp(level,1,3)-1
    const QVector<int> &l = alg.BrightEqLumaLut(idx);
    for (int i = 0; i < 512; ++i) {
        const unsigned lo = (2*i     < l.size()) ? unsigned(l[2*i])     & 0xfff : 0u;
        const unsigned hi = (2*i + 1 < l.size()) ? unsigned(l[2*i + 1]) & 0xfff : 0u;
        WriteValueToPL(REG_BRIGHT_EQ_LUMA_LUT + unsigned(i) * 4, (hi << 16) | lo);
    }
}

void KPlControl::SetAECAndAGCValue(unsigned int aec, unsigned int agc)
{
    // Реф.: w2 = (aec & 0xffff) | (agc << 16).
    const unsigned int v = (aec & 0xffff) | (agc << 16);
    WriteValueToPL(REG_AEC_AGC, v);
}

void KPlControl::SetAWBValue(unsigned int rGain, unsigned int bGain)
{
    // Реф.: w2 = (bGain & 0x1ffff) | (rGain << 16); строб REG_AWB_STROBE 1→0.
    const unsigned int v = (bGain & 0x1ffff) | (rGain << 16);
    WriteValueToPL(REG_AWB_GAIN, v);
    WriteValueToPL(REG_AWB_STROBE, 1);
    usleep(10);
    WriteValueToPL(REG_AWB_STROBE, 0);
}

void KPlControl::SetAwbCut(int low, int high)
{
    // Реф.: w2 = low | (high << 16).
    const unsigned int v = static_cast<unsigned int>(low)
                         | (static_cast<unsigned int>(high) << 16);
    WriteValueToPL(REG_AWB_CUT, v);
}

void KPlControl::SetVistSwitch(bool enable)
{
    // Реф.: REG_VIST_SWITCH = en; REG_AWB_START = !en (взаимоисключение с AWB-трактом).
    WriteValueToPL(REG_VIST_SWITCH, enable ? 1u : 0u);
    WriteValueToPL(REG_AWB_START, enable ? 0u : 1u);
}

void KPlControl::SetVistMatrix(const unsigned int *data, int count)
{
    if (!data || count <= 0) return;
    // Реф.: пары значений → REG_VIST_MATRIX.. ; последний (нечётный) → REG_VIST_TAIL (16-бит).
    unsigned long reg = REG_VIST_MATRIX;
    for (int i = 0; i + 1 < count; i += 2, reg += 4)
        WriteValueToPL(reg, data[i] | (data[i + 1] << 16));
    // хвостовой коэффициент
    WriteValueToPL(REG_VIST_TAIL, data[count - 1] & 0xffff);
}

void KPlControl::SetDenoiseLevel(int level)
{
    WriteValueToPL(REG_DENOISE_LEVEL, static_cast<unsigned int>(level));
}

namespace {
// Записать 4 банка по count значений: база + i*4, банки со смещением DENOISE_BANK_STEP.
// Источник — плоский буфер src, окно банка = count (за пределами — 0).
void writeDenoiseBank(KPlControl *pl, unsigned long base, const int *src,
                      int srcCount, int count)
{
    for (int bank = 0; bank < 4; ++bank) {
        const unsigned long bbase = base + static_cast<unsigned long>(bank) * DENOISE_BANK_STEP;
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
    WriteValueToPL(REG_GAMMA_CTRL, enable ? 1u : 0u);
}

void KPlControl::SetZoomValue(unsigned int value)
{
    WriteValueToPL(REG_ZOOM_VALUE, value);
}

void KPlControl::SetCCM1(int enable)
{
    WriteValueToPL(REG_CCM1_ENABLE, static_cast<unsigned int>(enable));
}

void KPlControl::SetCCM1Matrix(const unsigned int *data, int count)
{
    // Реф. SetCCM1Matrix (дизасм X2000): пары data[2i]|(data[2i+1]<<16) → REG_CCM1_MATRIX..;
    // затем хвост — последний коэффициент как 16-бит (ldurh) → REG_CCM1_TAIL.
    if (!data || count <= 0) return;
    unsigned long reg = REG_CCM1_MATRIX;
    for (int i = 0; i + 1 < count; i += 2, reg += 4)
        WriteValueToPL(reg, data[i] | (data[i + 1] << 16));
    WriteValueToPL(REG_CCM1_TAIL, data[count - 1] & 0xffff);
}

void KPlControl::SetChbStatus(int status)
{
    // Реф. SetChbStatus: status==0 → выкл (REG_CHB_ENABLE=0); иначе → вкл (=1) +
    // запись CHb-значения (4-е из CHb-файла, реф. AlgParaManager+0x7a3c) в REG_CHB_VALUE.
    if (status == 0) {
        WriteValueToPL(REG_CHB_ENABLE, 0);
    } else {
        WriteValueToPL(REG_CHB_ENABLE, 1);
        WriteValueToPL(REG_CHB_VALUE,
                       static_cast<unsigned int>(AlgParaManager::GetInstance().ChbValue()));
    }
}

bool KPlControl::GetFpga1Version(unsigned int &version)
{
    version = 0;
    return ReadValueFromPL(REG_FPGA1_VERSION, version);
}

bool KPlControl::GetFpga2Version(unsigned int &version)
{
    version = 0;
    return ReadValueFromPL(REG_FPGA_VERSION, version);
}

bool KPlControl::GetFpga3Version(unsigned int &version)
{
    version = 0;
    return ReadValueFromPL(REG_FPGA0_VERSION, version);
}

bool KPlControl::GetFpga2System(unsigned int &value)
{
    value = 0;
    return ReadValueFromPL(REG_PL_STATUS, value);
}

bool KPlControl::ReadAWBValue(unsigned int &rGain, unsigned int &bGain)
{
    // Реф. ReadAWBValue (дизасм X2000, REG_AWB_VALUE): распаковка — 14 бит на канал:
    //   bGain = v & 0x3fff (and w0,w4,#0x3fff); rGain = (v>>16) & 0x3fff (ubfx #16,#14).
    unsigned int v = 0;
    if (!ReadValueFromPL(REG_AWB_VALUE, v))
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
    WriteValueToPL(REG_FREEZE_STATUS, static_cast<unsigned int>(status));
}

void KPlControl::SetVideoDisplay(int mode)
{
    WriteValueToPL(REG_DISPLAY_MODE, static_cast<unsigned int>(mode));
}

void KPlControl::SetFreezeScalerIn(int a, int b)
{
    WriteValueToPL(REG_FREEZE_SCALER_IN,
                   static_cast<unsigned int>(a) | (static_cast<unsigned int>(b) << 16));
}

void KPlControl::SetFreezeScalerOut(int a, int b)
{
    WriteValueToPL(REG_FREEZE_SCALER_OUT,
                   static_cast<unsigned int>(a) | (static_cast<unsigned int>(b) << 16));
}

void KPlControl::SetFreezeScalerRatio(int a, int b)
{
    WriteValueToPL(REG_FREEZE_SCALER_RATIO,
                   static_cast<unsigned int>(a) | (static_cast<unsigned int>(b) << 16));
}

void KPlControl::SetCutPara(int a, int b)
{
    // Реф.: value = b | (a<<16).
    WriteValueToPL(REG_CUT_PARA,
                   static_cast<unsigned int>(b) | (static_cast<unsigned int>(a) << 16));
}

void KPlControl::SetFreezeVideoLoc(int a, int b, int c, int d)
{
    // Реф. SetFreezeVideoLoc: REG_FREEZE_LOC_A = a|(b<<16), FreezeLocB = c|(d<<16).
    WriteValueToPL(REG_FREEZE_LOC_A,
                   static_cast<unsigned int>(a) | (static_cast<unsigned int>(b) << 16));
    WriteValueToPL(REG_FREEZE_LOC_B,
                   static_cast<unsigned int>(c) | (static_cast<unsigned int>(d) << 16));
}

void KPlControl::SetLensSize(int a, int b)
{
    // Реф. SetLensSize: REG_LENS_SIZE = a|(b<<16).
    WriteValueToPL(REG_LENS_SIZE,
                   static_cast<unsigned int>(a) | (static_cast<unsigned int>(b) << 16));
}

void KPlControl::SetEnhanceSize(int, int)
{
    // Реф. SetEnhanceSize — пустая функция (только ret) в этой прошивке.
}

void KPlControl::SetDemoireEN(int value)
{
    // Реф. SetDemoireEN: REG_DEMOIRE_EN = value (passthrough).
    WriteValueToPL(REG_DEMOIRE_EN, static_cast<unsigned int>(value));
}

void KPlControl::SetContrastLevel(int)
{
    // Реф. SetContrastLevel — пустая функция (только ret) в этой прошивке.
}

void KPlControl::SetSensorRLut(const unsigned int *data, int count)
{
    writePairedLut(this, REG_SENSOR_LUT_R, data, count);
}

void KPlControl::SetSensorGLut(const unsigned int *data, int count)
{
    writePairedLut(this, REG_SENSOR_LUT_G, data, count);
}

void KPlControl::SetSensorBLut(const unsigned int *data, int count)
{
    writePairedLut(this, REG_SENSOR_LUT_B, data, count);
}

void KPlControl::SetRbcLut()
{
    // Реф. SetRbcLut (void): три канала из AlgParaManager (массив, word 0x4e8/0x507/
    // 0x526, по 31 значению) в соседние банки региона REG_RBC_S, по слову на i.
    const AlgParaManager::RbcLut &rb = AlgParaManager::GetInstance().CurRbcLut();
    const int count = qMin(rb.hb.size(), qMin(rb.hr.size(), rb.s.size()));
    for (int i = 0; i < count; ++i) {
        const unsigned long off = static_cast<unsigned long>(i) * 4;
        WriteValueToPL(REG_RBC_HB + off, rb.hb[i]);
        WriteValueToPL(REG_RBC_HR + off, rb.hr[i]);
        WriteValueToPL(REG_RBC_S  + off, rb.s[i]);
    }
}

void KPlControl::SetKneeLut()
{
    // Реф. SetKneeLut (void): значения (10 бит) из AlgParaManager (массив, word 0x94b;
    // число — word 0xd4b, min с 1024) парами → 3 банка; затем защёлка бит1.
    const QVector<int> &data = AlgParaManager::GetInstance().CurKneeLut();
    int count = data.size();
    if (count <= 0) return;
    if (count > 1024) count = 1024;
    const int pairs = count / 2;
    for (int i = 0; i < pairs; ++i) {
        const unsigned lo = static_cast<unsigned>(data[2*i])     & 0x3ff;
        const unsigned hi = static_cast<unsigned>(data[2*i + 1]) & 0x3ff;
        const unsigned v  = (hi << 16) | lo;
        const unsigned long off = static_cast<unsigned long>(i) * 4;
        WriteValueToPL(REG_KNEE_BANK0 + off, v);
        WriteValueToPL(REG_KNEE_BANK1 + off, v);
        WriteValueToPL(REG_KNEE_BANK2 + off, v);
    }
    // Финализация: REG_KNEE_CTRL |= LatchBit (read-modify-write, защёлка LUT).
    unsigned int ctrl = 0;
    ReadValueFromPL(REG_KNEE_CTRL, ctrl);
    WriteValueToPL(REG_KNEE_CTRL, ctrl | KNEE_LATCH_BIT);
}

void KPlControl::SetIrisTable(int shift)
{
    // Реф. SetIrisTable(int shift): данные из AlgParaManager (массив, указатель 0x7a48,
    // 8040 значений). 8 значений на регистр — (v>>shift) в ниббл k*4. REG_IRIS_TABLE,
    // count/8 записей (8040 → 1005).
    const QVector<int> &data = AlgParaManager::GetInstance().CurIrisTable();
    const int count = data.size();
    if (count < 8) return;
    const int regs = count / 8;
    for (int r = 0; r < regs; ++r) {
        unsigned int v = 0;
        for (int k = 0; k < 8; ++k)
            v |= (static_cast<unsigned int>(data[r * 8 + k]) >> shift) << (k * 4);
        WriteValueToPL(REG_IRIS_TABLE + static_cast<unsigned long>(r) * 4, v);
    }
}

void KPlControl::SetRealtimeVideoState(int state)
{
    WriteValueToPL(REG_DISPLAY_REALTIME, static_cast<unsigned int>(state));
}

void KPlControl::SetApmAreaDisplay(bool show)
{
    WriteValueToPL(REG_APM_AREA_SHOW, show ? 1u : 0u);
}

void KPlControl::VideoTest(int mode)
{
    // Реф.: значение сдвигается влево на 2 (mode<<2).
    WriteValueToPL(REG_VIDEO_TEST, static_cast<unsigned int>(mode) << 2);
}

void KPlControl::StartAWB()
{
    // Реф. StartAWB: взвести триггер AWB (REG_AWB_START) и снять VIST-переключатель.
    WriteValueToPL(REG_AWB_START, 1);
    WriteValueToPL(REG_VIST_SWITCH, 0);
}

int KPlControl::ReadIrisValue()
{
    // Реф.: читаем REG_IRIS_VALUE, возвращаем младший байт.
    unsigned int v = 0;
    ReadValueFromPL(REG_IRIS_VALUE, v);
    return static_cast<int>(v & 0xff);
}

void KPlControl::SetCornerCutWay(int a, int b, int c)
{
    // Реф. SetCornerCutWay: AlgParaManager::SetCutCornerPara(a,b,c) считает LUT угла,
    // затем стрим kCutCornerLen(=1080) слов из выбранного банка (a) в REG_CORNER_CUT_LUT.
    auto &alg = AlgParaManager::GetInstance();
    alg.SetCutCornerPara(a, b, c);
    const QVector<int> &lut = alg.CutCornerLut(a);
    const int n = qMin(lut.size(), AlgParaManager::kCutCornerLen);
    for (int i = 0; i < n; ++i)
        WriteValueToPL(REG_CORNER_CUT_LUT + static_cast<unsigned long>(i) * 4,
                       static_cast<unsigned int>(lut[i]));
}

void KPlControl::SetAuroraOffset(unsigned char a, unsigned char b)
{
    // Реф.: value = (a & 0xff) | ((b & 0xff) << 8).
    WriteValueToPL(REG_AURORA_OFFSET, static_cast<unsigned int>(a) | (static_cast<unsigned int>(b) << 8));
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
    WriteValueToPL(REG_CAPTURE_AREA, v);
}

void KPlControl::SetVideoArea(int width, int height)
{
    // Реф.: передать размеры rect в AlgParaManager::resize (без прямой записи в PL).
    AlgParaManager::GetInstance().resize(width, height);
}

void KPlControl::SetCameraIrisType(int type, int subtype)
{
    // Реф. SetCameraIrisType: кодирование по type/subtype, флаг 0x30, → REG_IRIS_CAMERA_TYPE.
    unsigned int v;
    if (type == 2) {
        const unsigned int mode = (subtype == 5) ? 0x100u : 0x200u;
        v = (2u & 0x3) | mode | 0x30u;
    } else {
        const unsigned int hi = (type == 0) ? 0x500u : 0x400u;
        v = (static_cast<unsigned int>(type) & 0x3) | hi | 0x30u;
    }
    WriteValueToPL(REG_IRIS_CAMERA_TYPE, v);
}

void KPlControl::ReadBrightnessHistogramValue(unsigned short *out, int count)
{
    if (!out || count <= 0) return;
    // Реф. ReadBrightnessHistogramValue (дизасм): триггер=1 (REG_HIST_TRIGGER), чтение
    // бинов (REG_HIST_BINS+, по 2 uint16 в 32-бит слове: low→out[2i], high→out[2i+1]),
    // затем сброс триггера=0.
    WriteValueToPL(REG_HIST_TRIGGER, 1);
    const int words = count / 2;
    for (int i = 0; i < words; ++i) {
        unsigned int w = 0;
        ReadValueFromPL(REG_HIST_BINS + static_cast<unsigned long>(i) * 4, w);
        out[2 * i]     = static_cast<unsigned short>(w & 0xffff);
        out[2 * i + 1] = static_cast<unsigned short>((w >> 16) & 0xffff);
    }
    WriteValueToPL(REG_HIST_TRIGGER, 0);   // реф.: сброс триггера после чтения
}

void KPlControl::SetDenoiseLut()
{
    // Реф. SetDenoiseLut (void) — читает AlgParaManager (массив по офсетам 0x3558.. и
    // блокам 0x11bc/0x1158/0xd5a). Карта регистров (ctrl/KPlRegs.h):
    //   заголовок dpc → REG_DENOISE_DPC (4 банка +BANK_STEP);
    //   kernelG → REG_DENOISE_KERNEL_G (41×4 банка); kernelRB → REG_DENOISE_KERNEL_RB
    //   (25×4); Lut → REG_DENOISE_LUT (256×4).
    // Банки в оригинале берут смежные окна одного буфера; здесь источник — плоские
    // массивы (kernelG 42 / kernelRB 25 / lut 256), лишние банки добиваются нулём.
    const AlgParaManager::DenoisePlData &d = AlgParaManager::GetInstance().CurDenoise();
    for (int i = 0; i < 4; ++i)
        WriteValueToPL(REG_DENOISE_DPC + static_cast<unsigned long>(i) * DENOISE_BANK_STEP,
                       static_cast<unsigned int>(d.dpc[i]));
    writeDenoiseBank(this, REG_DENOISE_KERNEL_G,  d.kernelG.constData(),  d.kernelG.size(),  41);
    writeDenoiseBank(this, REG_DENOISE_KERNEL_RB, d.kernelRB.constData(), d.kernelRB.size(), 25);
    writeDenoiseBank(this, REG_DENOISE_LUT,       d.lut.constData(),      d.lut.size(),      256);
}
