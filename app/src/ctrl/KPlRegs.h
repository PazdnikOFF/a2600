#pragma once

// ============================================================================
//  Карта регистров программируемой логики (PL / FPGA) стойки X-2600.
//
//  Восстановлена из дизассемблера оригинала (X2000). Плоские #define-макросы
//  (классический стиль эмбеддед-прошивок): адресное пространство PL разбито на
//  аппаратные блоки с базой 0xa1_8X_0000 (и группу видео-фронта 0xa0_0X_xxxx);
//  доступ — KPlControl::WriteValueToPL/ReadValueFromPL(addr, value) через /dev/mem.
//
//  Имена: REG_<БЛОК>_<НАЗНАЧЕНИЕ>. НЕ менять значения без сверки с бинарником —
//  self-test `plreg` проверяет точные адреса.
//
//  ПРИМЕЧАНИЕ: оригинал скомпилирован без отладочной информации приложения
//  (DWARF только для glibc-старта), поэтому его внутренняя раскладка констант/
//  имена макросов НЕвосстановимы. Восстановимы и совпадают: имена класса/методов
//  (KPlControl::Set*/Read*) и сами адреса регистров. Эта карта — реконструкция
//  для читаемости; на поведение и публичную структуру не влияет.
// ============================================================================

// --- Видео-фронт FPGA0 (AEC/AGC, Aurora-serdes, тест-паттерн, версии) ---------
#define REG_AEC_AGC          0xa0048020UL   // SetAECAndAGCValue: bGain|(rGain<<16)
#define REG_AURORA_OFFSET    0xa004a02cUL   // SetAuroraOffset: a|(b<<8)
#define REG_VIDEO_TEST       0xa004a040UL   // VideoTest: mode<<2
#define REG_FPGA1_VERSION    0xa004a044UL   // GetFpga1Version (чтение)
#define REG_FPGA0_VERSION    0xa0060000UL   // GetFpga3Version (чтение)

// --- Вывод видео -------------------------------------------------------------
#define REG_DISPLAY_REALTIME 0xa0080024UL   // SetRealtimeVideoState
#define REG_DISPLAY_MODE     0xa0080028UL   // SetVideoDisplay

// --- Главный контроль/статус PL ----------------------------------------------
#define REG_FPGA_VERSION     0xa1000000UL   // GetFpga2Version (чтение)
#define REG_PL_STATUS        0xa1000008UL   // GetFpga2System — статус PL (чтение)
#define REG_AWB_STROBE       0xa100019cUL   // SetAWBValue: строб 1→0

// --- Геометрия/фриз кадра ----------------------------------------------------
#define REG_FREEZE_LOC_A     0xa1800024UL   // SetFreezeVideoLoc: a|(b<<16)
#define REG_FREEZE_LOC_B     0xa1800028UL   // SetFreezeVideoLoc: c|(d<<16)
#define REG_FREEZE_STATUS    0xa180002cUL   // SetFreezeStatus

// --- LUT линеаризации сенсора (SetSensorR/G/BLut). Пары значений --------------
#define REG_SENSOR_LUT_R     0xa1820800UL
#define REG_SENSOR_LUT_G     0xa1821000UL
#define REG_SENSOR_LUT_B     0xa1821800UL

// --- Гамма (SetGammaLut/SetGammaEnable). Ctrl-регистр + 3 банка (сдвиг 0x800) --
#define REG_GAMMA_CTRL       0xa1830000UL   // enable + защёлка (бит1) + poll(бит2)
#define REG_GAMMA_BANK0      0xa1830800UL
#define REG_GAMMA_BANK1      0xa1831000UL
#define REG_GAMMA_BANK2      0xa1831800UL
#define GAMMA_LATCH_BIT      0x2u           // REG_GAMMA_CTRL |= — применить LUT

// --- Баланс белого (AWB) -----------------------------------------------------
#define REG_AWB_START        0xa1840000UL   // StartAWB=1; SetVistSwitch пишет !en
#define REG_AWB_GAIN         0xa184000cUL   // SetAWBValue: bGain|(rGain<<16)
#define REG_AWB_VALUE        0xa1840014UL   // ReadAWBValue (чтение, 14 бит/канал)
#define REG_AWB_CUT          0xa1840018UL   // SetAwbCut

// --- Улучшение изображения / демуар ------------------------------------------
#define REG_IMAGE_ENH        0xa1850058UL   // SetImageEnhValue
#define REG_DEMOIRE_EN       0xa18501ccUL   // SetDemoireEN

// --- Цветокоррекция CCM0 (+ CutPara). Пары коэф. + 16-бит хвост ---------------
#define REG_CCM0_ENABLE      0xa1860000UL   // SetCCM0
#define REG_CCM0_MATRIX      0xa1860004UL   // пары m[2i]|(m[2i+1]<<16): 0x04/08/0c/10
#define REG_CCM0_TAIL        0xa1860014UL   // 9-й коэффициент (16-бит)
#define REG_CUT_PARA         0xa1860018UL   // SetCutPara

// --- Тональность RBC (SetToneValue) ------------------------------------------
#define REG_TONE_C           0xa1870000UL   // общий
#define REG_TONE_R           0xa1870004UL
#define REG_TONE_B           0xa1870008UL

// --- RBC-LUT сосудистого контраста (SetRbcLut). 3 канала, по слову на i --------
#define REG_RBC_S            0xa1878000UL
#define REG_RBC_HR           0xa1878100UL
#define REG_RBC_HB           0xa1878200UL

// --- Цветокоррекция CCM1 (SetCCM1/Matrix). Пары + 16-бит хвост ----------------
#define REG_CCM1_ENABLE      0xa1880000UL   // SetCCM1
#define REG_CCM1_MATRIX      0xa1880004UL   // пары: 0x04/08/0c/10
#define REG_CCM1_TAIL        0xa1880014UL   // последний коэффициент (16-бит)

// --- Оптика/линза ------------------------------------------------------------
#define REG_LENS_SIZE        0xa189000cUL   // SetLensSize: a|(b<<16)

// --- Диафрагма/камера/гистограмма яркости ------------------------------------
#define REG_IRIS_CAMERA_TYPE 0xa18a0000UL   // SetCameraIrisType
#define REG_IRIS_VALUE       0xa18a0004UL   // ReadIrisValue (чтение, младший байт)
#define REG_APM_AREA_SHOW    0xa18a0008UL   // SetApmAreaDisplay
#define REG_HIST_TRIGGER     0xa18a0010UL   // ReadBrightnessHistogram: триггер 1→0
#define REG_IRIS_TABLE       0xa18a8000UL   // SetIrisTable (8 нибблов/регистр)
#define REG_HIST_BINS        0xa18a9000UL   // бины гистограммы (2×uint16/слово)

// --- Обрезка углов (SetCornerCutWay). Стрим 1080-элем. LUT --------------------
#define REG_CORNER_CUT_LUT   0xa18c8000UL   // 0xa18c8000..0xa18c90dc

// --- Зум / область захвата ---------------------------------------------------
#define REG_ZOOM_VALUE       0xa18d0004UL   // SetZoomValue
#define REG_CAPTURE_AREA     0xa18d0008UL   // SetVideoCaptureArea

// --- VIST/SFI спектральная матрица (SetVistSwitch/Matrix). Пары + хвост -------
#define REG_VIST_SWITCH      0xa18e0000UL   // SetVistSwitch: en
#define REG_VIST_MATRIX      0xa18e0004UL   // пары значений
#define REG_VIST_TAIL        0xa18e0014UL   // последний (16-бит)

// --- Цветоусиление (SetColorEnhParam) ----------------------------------------
#define REG_COLOR_ENH_ENABLE 0xa18f0008UL   // SetColorEnhParam enable
#define REG_COLOR_ENH_PARAM  0xa18f0024UL   // значение уровня

// --- CHb (усиление гемоглобина, SetChbStatus) --------------------------------
#define REG_CHB_ENABLE       0xa1900008UL   // строб вкл/выкл
#define REG_CHB_VALUE        0xa1900018UL   // 4-е значение CHb-файла

// --- Скейлер фриза (SetFreezeScaler*) ----------------------------------------
#define REG_FREEZE_SCALER_RATIO 0xa1910008UL // SetFreezeScalerRatio
#define REG_FREEZE_SCALER_IN    0xa191000cUL // SetFreezeScalerIn
#define REG_FREEZE_SCALER_OUT   0xa1910010UL // SetFreezeScalerOut

// --- Knee-LUT (SetKneeLut). Ctrl + 3 банка (сдвиг 0x800) ----------------------
#define REG_KNEE_CTRL        0xa1930000UL   // защёлка (бит1) + poll(бит2)
#define REG_KNEE_BANK0       0xa1930800UL
#define REG_KNEE_BANK1       0xa1931000UL
#define REG_KNEE_BANK2       0xa1931800UL
#define KNEE_LATCH_BIT       0x2u

// --- Шумоподавление (SetDenoiseLevel/SetDenoiseLut). 4 банка (сдвиг 0x1000) ----
#define REG_DENOISE_LEVEL    0xa1940008UL   // SetDenoiseLevel
#define REG_DENOISE_DPC      0xa1941010UL   // dpc-пороги (4 банка)
#define REG_DENOISE_LUT      0xa1941100UL   // LUT 256×4 банка
#define REG_DENOISE_KERNEL_RB 0xa1941500UL  // ядро R/B 25×4
#define REG_DENOISE_KERNEL_G 0xa1941600UL   // ядро G 41×4
#define DENOISE_BANK_STEP    0x1000UL       // шаг между 4 банками

// --- Bright EQ (SetBrightEQEnable/Lut) ---------------------------------------
#define REG_BRIGHT_EQ_ENABLE   0xa1950000UL // SetBrightEQEnable
#define REG_BRIGHT_EQ_GAUSSIAN 0xa1950004UL // гауссов фильтр (18 записей)
#define REG_BRIGHT_EQ_LUMA_LUT 0xa1958000UL // lumaGainLut (512 записей)
