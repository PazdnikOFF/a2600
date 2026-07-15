#pragma once

// ============================================================================
//  Карта регистров программируемой логики (PL / FPGA) стойки X-2600.
//
//  Восстановлена из дизассемблера оригинала (X2000). Аналог вендорских
//  register-map заголовков (ARM CMSIS / SVD): адресное пространство PL разбито
//  на аппаратные блоки с базой 0xa1_8X_0000 (и группу видео-фронта 0xa0_0X_xxxx);
//  внутри блока — регистры со смещениями. Доступ — KPlControl::WriteValueToPL/
//  ReadValueFromPL(addr, value) через /dev/mem (KMemDevice).
//
//  Правило: каждый регистр = <Блок>::Base + смещение. Имена смещений отражают
//  назначение, чтобы код читался без обращения к дизасму. НЕ менять значения без
//  сверки с бинарником — self-test `plreg` проверяет точные адреса.
// ============================================================================

namespace plreg {

using addr_t = unsigned long;

// --- Видео-фронт FPGA0 (AEC/AGC, Aurora-serdes, тест-паттерн, версии) ---------
namespace Front {
    constexpr addr_t AecAgc      = 0xa0048020;  // SetAECAndAGCValue: bGain|(rGain<<16)
    constexpr addr_t AuroraOffset= 0xa004a02c;  // SetAuroraOffset: a|(b<<8)
    constexpr addr_t VideoTest   = 0xa004a040;  // VideoTest: mode<<2
    constexpr addr_t Fpga1Version= 0xa004a044;  // GetFpga1Version (чтение)
    constexpr addr_t Fpga0Version= 0xa0060000;  // GetFpga0Version (чтение)
}

// --- Вывод видео -------------------------------------------------------------
namespace Display {
    constexpr addr_t RealtimeState = 0xa0080024;  // SetRealtimeVideoState
    constexpr addr_t Mode          = 0xa0080028;  // SetVideoDisplay
}

// --- Главный контроль/статус PL ----------------------------------------------
namespace Ctrl {
    constexpr addr_t Version   = 0xa1000000;  // GetFpgaVersion (чтение)
    constexpr addr_t Status    = 0xa1000008;  // статус PL (чтение)
    constexpr addr_t AwbStrobe = 0xa100019c;  // SetAWBValue: строб 1→0
}

// --- Геометрия/фриз кадра ----------------------------------------------------
namespace Video {
    constexpr addr_t Base         = 0xa1800000;
    constexpr addr_t FreezeLocA   = Base + 0x24;  // SetFreezeVideoLoc: a|(b<<16)
    constexpr addr_t FreezeLocB   = Base + 0x28;  // SetFreezeVideoLoc: c|(d<<16)
    constexpr addr_t FreezeStatus = Base + 0x2c;  // SetFreezeStatus
}

// --- LUT линеаризации сенсора (SetSensorR/G/BLut). Пары значений --------------
namespace SensorLut {
    constexpr addr_t Base = 0xa1820000;
    constexpr addr_t R    = Base + 0x0800;
    constexpr addr_t G    = Base + 0x1000;
    constexpr addr_t B    = Base + 0x1800;
}

// --- Гамма (SetGammaLut/SetGammaEnable). Ctrl-регистр + 3 банка со сдвигом 0x800
namespace Gamma {
    constexpr addr_t Base   = 0xa1830000;
    constexpr addr_t Ctrl   = Base + 0x0000;  // enable + защёлка (бит1) + poll(бит2)
    constexpr addr_t Bank0  = Base + 0x0800;
    constexpr addr_t Bank1  = Base + 0x1000;
    constexpr addr_t Bank2  = Base + 0x1800;
    constexpr addr_t LatchBit = 0x2;          // Ctrl |= LatchBit — применить LUT
}

// --- Баланс белого (AWB) -----------------------------------------------------
namespace Awb {
    constexpr addr_t Base  = 0xa1840000;
    constexpr addr_t Start = Base + 0x00;  // StartAWB=1; SetVistSwitch пишет !en
    constexpr addr_t Gain  = Base + 0x0c;  // SetAWBValue: bGain|(rGain<<16)
    constexpr addr_t Value = Base + 0x14;  // ReadAWBValue (чтение, 14 бит/канал)
    constexpr addr_t Cut   = Base + 0x18;  // SetAwbCut
}

// --- Улучшение изображения / демуар ------------------------------------------
namespace Image {
    constexpr addr_t Base       = 0xa1850000;
    constexpr addr_t EnhValue   = Base + 0x0058;  // SetImageEnhValue
    constexpr addr_t DemoireEn  = Base + 0x01cc;  // SetDemoireEN
}

// --- Цветокоррекция CCM0 (+ CutPara). Пары коэф. + 16-бит хвост ---------------
namespace Ccm0 {
    constexpr addr_t Base    = 0xa1860000;
    constexpr addr_t Enable  = Base + 0x00;  // SetCCM0
    constexpr addr_t Matrix  = Base + 0x04;  // пары m[2i]|(m[2i+1]<<16): 0x04/08/0c/10
    constexpr addr_t Tail    = Base + 0x14;  // 9-й коэффициент (16-бит)
    constexpr addr_t CutPara = Base + 0x18;  // SetCutPara
}

// --- Тональность RBC (SetToneValue) ------------------------------------------
namespace Tone {
    constexpr addr_t Base = 0xa1870000;
    constexpr addr_t C    = Base + 0x0;  // общий
    constexpr addr_t R    = Base + 0x4;
    constexpr addr_t B    = Base + 0x8;
}

// --- RBC-LUT сосудистого контраста (SetRbcLut). 3 канала, по слову на i --------
namespace Rbc {
    constexpr addr_t Base = 0xa1878000;
    constexpr addr_t S    = Base + 0x000;  // 0xa1878000
    constexpr addr_t Hr   = Base + 0x100;  // 0xa1878100
    constexpr addr_t Hb   = Base + 0x200;  // 0xa1878200
}

// --- Цветокоррекция CCM1 (SetCCM1/Matrix). Пары + 16-бит хвост ----------------
namespace Ccm1 {
    constexpr addr_t Base   = 0xa1880000;
    constexpr addr_t Enable = Base + 0x00;  // SetCCM1
    constexpr addr_t Matrix = Base + 0x04;  // пары: 0x04/08/0c/10
    constexpr addr_t Tail   = Base + 0x14;  // последний коэффициент (16-бит)
}

// --- Оптика/линза ------------------------------------------------------------
namespace Lens {
    constexpr addr_t Base = 0xa1890000;
    constexpr addr_t Size = Base + 0x0c;  // SetLensSize: a|(b<<16)
}

// --- Диафрагма/камера/гистограмма яркости ------------------------------------
namespace Iris {
    constexpr addr_t Base          = 0xa18a0000;
    constexpr addr_t CameraType    = Base + 0x0000;  // SetCameraIrisType
    constexpr addr_t Value         = Base + 0x0004;  // ReadIrisValue (чтение, младший байт)
    constexpr addr_t ApmAreaShow   = Base + 0x0008;  // SetApmAreaDisplay
    constexpr addr_t HistTrigger   = Base + 0x0010;  // ReadBrightnessHistogram: триггер 1→0
    constexpr addr_t Table         = Base + 0x8000;  // SetIrisTable (8 нибблов/регистр)
    constexpr addr_t HistBins      = Base + 0x9000;  // бины гистограммы (2×uint16/слово)
}

// --- Обрезка углов (SetCornerCutWay). Стрим 1080-элем. LUT --------------------
namespace CornerCut {
    constexpr addr_t Base = 0xa18c0000;
    constexpr addr_t Lut  = Base + 0x8000;  // 0xa18c8000..0xa18c90dc
}

// --- Зум / область захвата ---------------------------------------------------
namespace Zoom {
    constexpr addr_t Base        = 0xa18d0000;
    constexpr addr_t Value       = Base + 0x04;  // SetZoomValue
    constexpr addr_t CaptureArea = Base + 0x08;  // SetVideoCaptureArea
}

// --- VIST/SFI спектральная матрица (SetVistSwitch/Matrix). Пары + хвост -------
namespace Vist {
    constexpr addr_t Base   = 0xa18e0000;
    constexpr addr_t Switch = Base + 0x00;  // SetVistSwitch: en
    constexpr addr_t Matrix = Base + 0x04;  // пары значений
    constexpr addr_t Tail   = Base + 0x14;  // последний (16-бит)
}

// --- Цветоусиление (SetColorEnhParam) ----------------------------------------
namespace ColorEnh {
    constexpr addr_t Base   = 0xa18f0000;
    constexpr addr_t Enable = Base + 0x08;  // SetColorEnhParam enable
    constexpr addr_t Param  = Base + 0x24;  // значение уровня
}

// --- CHb (усиление гемоглобина, SetChbStatus) --------------------------------
namespace Chb {
    constexpr addr_t Base   = 0xa1900000;
    constexpr addr_t Enable = Base + 0x08;  // строб вкл/выкл
    constexpr addr_t Value  = Base + 0x18;  // 4-е значение CHb-файла
}

// --- Скейлер фриза (SetFreezeScaler*) ----------------------------------------
namespace FreezeScaler {
    constexpr addr_t Base  = 0xa1910000;
    constexpr addr_t Ratio = Base + 0x08;  // SetFreezeScalerRatio
    constexpr addr_t In    = Base + 0x0c;  // SetFreezeScalerIn
    constexpr addr_t Out   = Base + 0x10;  // SetFreezeScalerOut
}

// --- Knee-LUT (SetKneeLut). Ctrl + 3 банка со сдвигом 0x800 -------------------
namespace Knee {
    constexpr addr_t Base     = 0xa1930000;
    constexpr addr_t Ctrl     = Base + 0x0000;  // защёлка (бит1) + poll(бит2)
    constexpr addr_t Bank0    = Base + 0x0800;
    constexpr addr_t Bank1    = Base + 0x1000;
    constexpr addr_t Bank2    = Base + 0x1800;
    constexpr addr_t LatchBit = 0x2;
}

// --- Шумоподавление (SetDenoiseLevel/SetDenoiseLut). 4 банка со сдвигом 0x1000
namespace Denoise {
    constexpr addr_t Base    = 0xa1940000;
    constexpr addr_t Level   = Base + 0x0008;  // SetDenoiseLevel
    constexpr addr_t BankStep= 0x1000;         // шаг между 4 банками
    constexpr addr_t Dpc     = Base + 0x1010;  // dpc-пороги (4 банка)
    constexpr addr_t Lut     = Base + 0x1100;  // LUT 256×4 банка
    constexpr addr_t KernelRB= Base + 0x1500;  // ядро R/B 25×4
    constexpr addr_t KernelG = Base + 0x1600;  // ядро G 41×4
}

// --- Bright EQ (SetBrightEQEnable/Lut) ---------------------------------------
namespace BrightEq {
    constexpr addr_t Base     = 0xa1950000;
    constexpr addr_t Enable   = Base + 0x0000;  // SetBrightEQEnable
    constexpr addr_t Gaussian = Base + 0x0004;  // гауссов фильтр (18 записей)
    constexpr addr_t LumaLut  = Base + 0x8000;  // lumaGainLut (512 записей)
}

} // namespace plreg
