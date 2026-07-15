#pragma once

#include <QObject>
#include <QVector>
#include <QPair>

#include "hal/KMemDevice.h"

// Доступ к регистрам FPGA (PL — Programmable Logic) через /dev/mem + mmap.
// Реф. класс KPlControl (X-2600). Сигнатуры и семантика — из дизассемблера:
//   WriteValueToPL(unsigned long physAddr, unsigned int value)   // _ZN10KPlControl14WriteValueToPLEmj
//   ReadValueFromPL(unsigned long physAddr, unsigned int &value) // _ZN10KPlControl15ReadValueFromPLEmRj
// Регион PL: 0xA00xxxxx (напр. v_tpg 0xa00a0000, регистр 0xa0080030).
// Оригинал: OpenPhyMem + mmap 32-битных регистров.
class KPlControl : public QObject
{
    Q_OBJECT
public:
    explicit KPlControl(QObject *parent = nullptr);
    ~KPlControl() override;

    bool WriteValueToPL(unsigned long physAddr, unsigned int value);
    bool ReadValueFromPL(unsigned long physAddr, unsigned int &value);

    // Загрузка гамма-LUT в PL (реф. KPlControl::SetGammaLut). База 0xa1830000,
    // три канала R/G/B со смещениями +0x000/+0x800/+0x1000, шаг записи 4 байта.
    void SetGammaLut(const QVector<int> &lut);

    // Загрузка матрицы цветокоррекции CCM0 в PL (реф. KPlControl::SetCCM0Matrix).
    // Регистры в регионе 0xa1860000; enable по 0xa1860014.
    void SetCCM0Matrix(const int m[9]);

    // --- Параметры изображения → PL (адреса/битовые поля из дизассемблера) ---

    // Цветоусиление (реф. SetColorEnhParam(bool,int)). enable→0xa18f0008,
    // значение уровня (из AlgParaManager, colenh_level.txt) →0xa18f0024.
    void SetColorEnhParam(bool enable, int level);

    // Усиление изображения (реф. SetImageEnhValue(int)). Значение уровня (из
    // AlgParaManager, ImgEnh/level_*.txt) →0xa1850058.
    void SetImageEnhValue(int level);

    // RBC/тон: три регистра региона 0xa1870000 (реф. SetColorR/B/C, SetToneValue).
    //   R-гейн →+0x4, B-гейн →+0x8, C (тон/насыщ.) →+0x0.
    void SetColorR(int value);            // 0xa1870004
    void SetColorB(int value);            // 0xa1870008
    void SetColorC(int value);            // 0xa1870000
    void SetToneValue(int r, int b, int c); // все три сразу (реф. SetToneValue)

    // Bright EQ включение (реф. SetBrightEQEnalbe(bool)) →0xa1950000.
    void SetBrightEQEnable(bool enable);

    // Bright EQ LUT (реф. SetBrightEQLut(int)). Два блока (данные из AlgParaManager):
    //   1) гауссов фильтр → 0xa1950004..0x004c (18 записей, пары 15-бит);
    //   2) lumaGainLut по уровню → 0xa1958000..0x8800 (512 записей, пары 12-бит).
    // level→индекс LUT: clamp(level,1,3)-1 (0=disable,1=low,2=middle).
    void SetBrightEQLut(int level);

    // AEC+AGC одним регистром (реф. SetAECAndAGCValue). 0xa0048020:
    //   биты [15:0]=AEC, [31:16]=AGC.
    void SetAECAndAGCValue(unsigned int aec, unsigned int agc);

    // Ручной баланс белого (реф. SetAWBValue). 0xa184000c: [16:0]=B-гейн,
    //   [31:16]=R-гейн; затем строб 0xa100019c 1→0 (usleep 10мкс).
    void SetAWBValue(unsigned int rGain, unsigned int bGain);

    // Пороги отсечки AWB (реф. SetAwbCut). 0xa1840018: [15:0]=low, [31:16]=high.
    void SetAwbCut(int low, int high);

    // VIST/SFI спектральная матрица (реф. SetVistMatrix). Регион 0xa18e0000:
    //   пары значений → 0xa18e0004.. (data[2i]|(data[2i+1]<<16)), хвост → 0xa18e0014.
    void SetVistMatrix(const unsigned int *data, int count);
    // Включение VIST (реф. SetVistSwitch): 0xa18e0000=en, 0xa1840000=!en.
    void SetVistSwitch(bool enable);

    // Уровень шумоподавления (реф. SetDenoiseLevel) → 0xa1940008.
    void SetDenoiseLevel(int level);
    // Загрузка LUT шумоподавления (реф. SetDenoiseLut). Регион 0xa1941000:
    //   заголовок dpc → 0xa194x010; kernelG → 0xa1941600.. (41×4 банка);
    //   kernelRB → 0xa1941500.. (25×4); Lut → 0xa1941100.. (256×4).
    struct DenoiseData {
        int dpc[4] = {0,0,0,0};
        const int *kernelG = nullptr;  int kernelGCount = 0;   // 42
        const int *kernelRB = nullptr; int kernelRBCount = 0;  // 25
        const int *lut = nullptr;      int lutCount = 0;       // 256
    };
    void SetDenoiseLut(const DenoiseData &d);

    // Включение гаммы (реф. SetGammaEnable) → 0xa1830000.
    void SetGammaEnable(bool enable);
    // Масштаб (реф. SetZoomValue) → 0xa18d0004.
    void SetZoomValue(unsigned int value);
    // Вторая матрица цветокоррекции (реф. SetCCM1/SetCCM1Matrix). Регион 0xa1880000:
    //   enable → 0xa1880000; коэффициенты парами → 0xa1880004..
    void SetCCM1(int enable);
    void SetCCM1Matrix(const unsigned int *data, int count);
    // CHb (усиление гемоглобина, реф. SetChbStatus): status==0 → выкл (0xa1900008=0);
    //   иначе → вкл (=1) + запись CHb-значения (из AlgParaManager) в 0xa1900018.
    void SetChbStatus(int status);
    // Версии/статус FPGA (реф. GetFpga*Version/GetFpga2System) — чтение регистров.
    bool GetFpga1Version(unsigned int &version);   // 0xa004a044
    bool GetFpga2Version(unsigned int &version);   // 0xa1000000
    bool GetFpga3Version(unsigned int &version);   // 0xa0060000
    bool GetFpga2System(unsigned int &value);      // 0xa1000008
    // Чтение результата ББ (реф. ReadAWBValue, дизасм): 0xa1840014 →
    //   rGain=(v>>16)&0x3fff, bGain=v&0x3fff (14 бит на канал).
    bool ReadAWBValue(unsigned int &rGain, unsigned int &bGain);

    // Стоп-кадр (реф. SetFreezeStatus) → 0xa180002c.
    void SetFreezeStatus(int status);
    // Дисплей видео (реф. SetVideoDisplay) → 0xa0080028.
    void SetVideoDisplay(int mode);
    // Скейлер стоп-кадра (реф. SetFreezeScalerIn/Out/Ratio) — регион 0xa1910000:
    //   In→+0xc, Out→+0x10, Ratio→+0x8; значение = a|(b<<16).
    void SetFreezeScalerIn(int a, int b);
    void SetFreezeScalerOut(int a, int b);
    void SetFreezeScalerRatio(int a, int b);
    // Параметры обрезки (реф. SetCutPara) → 0xa1860018 = b|(a<<16).
    void SetCutPara(int a, int b);
    // Позиция видео стоп-кадра (реф. SetFreezeVideoLoc): 0xa1800024 = a|(b<<16),
    //   0xa1800028 = c|(d<<16).
    void SetFreezeVideoLoc(int a, int b, int c, int d);
    // Размер линзы/маски (реф. SetLensSize) → 0xa189000c = a|(b<<16).
    void SetLensSize(int a, int b);
    // Реф. SetEnhanceSize/SetContrastLevel — в этой прошивке ПУСТЫЕ (только ret),
    //   оставлены для совместимости API (не пишут регистры).
    void SetEnhanceSize(int a, int b);
    void SetContrastLevel(int level);
    // Подавление муара (реф. SetDemoireEN) → 0xa18501cc = value (passthrough).
    void SetDemoireEN(int value);

    // LUT линеаризации сенсора по каналам (реф. SetSensorR/G/BLut). Значения из
    // sensor_lut_{r,g,b}.txt (через AlgParaManager). Регион 0xa1820000:
    //   R→+0x800, G→+0x1000, B→+0x1800; значения парами (data[2i]|(data[2i+1]<<16)).
    void SetSensorRLut(const unsigned int *data, int count);
    void SetSensorGLut(const unsigned int *data, int count);
    void SetSensorBLut(const unsigned int *data, int count);

    // RBC-LUT (реф. SetRbcLut) — 3 канала в регион 0xa1878000:
    //   Hb→0xa1878200, Hr→0xa1878100, S→0xa1878000 (по count значений, шаг 4).
    void SetRbcLut(const unsigned int *hb, const unsigned int *hr,
                   const unsigned int *s, int count);

    // Knee-LUT (реф. SetKneeLut). Значения (10 бит) пакуются парами и пишутся в
    //   3 банка региона 0xa1930000 (0x800/0x1000/0x1800); финализация 0xa1930000|=2.
    //   count — число значений (1024), даёт count/2 записей на банк.
    void SetKneeLut(const int *data, int count);

    // Таблица диафрагмы (реф. SetIrisTable). shift — план размера апертуры (0..2).
    //   Пакует по 8 значений (data[i]>>shift) в нибблы i*4 → 0xa18a8000.. (count/8 записей).
    void SetIrisTable(const int *data, int count, int shift);

    // Состояние реального видео (реф. SetRealtimeVideoState) → 0xa0080024.
    void SetRealtimeVideoState(int state);
    // Отображение области APM-метрирования (реф. SetApmAreaDisplay) → 0xa18a0008.
    void SetApmAreaDisplay(bool show);
    // Тестовая картинка FPGA (реф. VideoTest) → 0xa004a040 = mode<<2.
    void VideoTest(int mode);
    // Запуск автобаланса белого (реф. StartAWB): триггер 0xa1840000=1 + выкл VIST 0xa18e0000=0.
    // (В оригинале далее применяются гейны из AlgParaManager::getAwbPara — на устройстве.)
    void StartAWB();

    // Текущее значение диафрагмы (реф. ReadIrisValue) — младший байт 0xa18a0004.
    int ReadIrisValue();
    // Смещение Aurora-serdes (реф. SetAuroraOffset) → 0xa004a02c = a | (b<<8).
    void SetAuroraOffset(unsigned char a, unsigned char b);
    // Область захвата видео (реф. SetVideoCaptureArea) → 0xa18d0008. Точка (x,y)
    //   кодируется знак-величиной: enc(v)=((|v|*2)|(v<0?0x100:0))&0x1ff; y<<16.
    void SetVideoCaptureArea(int x, int y);
    // Тип диафрагмы камеры (реф. SetCameraIrisType) → 0xa18a0000. Кодировка:
    //   type 0→0x530, 1→0x431, 2→(2|(subtype==5?0x100:0x200)|0x30).
    void SetCameraIrisType(int type, int subtype);
    // Видео-область (реф. SetVideoArea(_VideoRect)): передаёт размеры в
    //   AlgParaManager::resize (downstream-параметры считаются от них).
    void SetVideoArea(int width, int height);
    // Гистограмма яркости (реф. ReadBrightnessHistogramValue): триггер 0xa18a0010=1,
    //   чтение 256 бинов (16-бит) из 0xa18a9000 (по 2 бина на слово). out — минимум 256.
    void ReadBrightnessHistogramValue(unsigned short *out, int count = 256);

    // --- Трассировка записей для off-device self-test (нет /dev/mem на Mac) ---
    using RegWrite = QPair<unsigned long, unsigned int>;
    void EnableTrace(bool on) { traceOn_ = on; }
    void ClearTrace() { trace_.clear(); }
    const QVector<RegWrite> &Trace() const { return trace_; }

private:
    KMemDevice mem_;            // низкоуровневый доступ к PL (реф. KMemDevice)
    bool traceOn_ = false;
    QVector<RegWrite> trace_;   // (адрес,значение) последних записей при traceOn_
};
