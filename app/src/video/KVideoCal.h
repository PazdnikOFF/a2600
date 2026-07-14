#pragma once

#include <QRect>
#include <QPair>

// Ядро калибровки видео (реф. класс KVideoCal, X-2600).
//
// В оригинале KVideoCal — Qt-диалог (QDialog: InitWidget/retranslateUi/спинбоксы),
// но его прикладное ядро off-device — это:
//   • диапазоны смещения центра по типу прошивки сенсора (GetCenterOffset*Range),
//   • дефолты «обрезки углов» кадра (GetDefaultVideoCornerCutting — round/octangle),
//   • сохранение области отображения в display-ini (SaveDisplayArea → KDisplayOption).
// UI-часть (спинбоксы/кнопки) и запись в железо (KEndoScope/KCamera::SetVideoCentorPoint,
// SetVideoCapArea) — Фаза E/F (нужен прибор), здесь не реализуются.
//
// Значения — 1:1 с бинарником X2000 (GetCenterOffsetHorizontalRange/VerticalRange).
class KVideoCal
{
public:
    // Тип прошивки сенсора (реф. _EndoFirmwareType). Упрощённая карта
    // config2firmwareTypeMap (строка firmwareType из video.ini → это значение).
    // Именно её принимают GetCenterOffset*Range.
    enum EndoFirmwareType {
        FW_OV2740          = 0,  // OV2740 (базовый)
        FW_OH01A_928X768   = 1,  // OH01A 928x768
        FW_IMX274          = 2,  // IMX274 1920x1080
        FW_OH01A_768X928   = 3,  // OH01A 768x928
        FW_OV6946          = 4,  // OV6946 400x400
        FW_OV2740_1280X960 = 5,  // OV2740 1280x960
        FW_OCHFA_720X720   = 7,  // OCHFA/OAH0428 720x720
        FW_OV2740_1024X1024= 8,  // OV2740 1024x1024
    };

    // Режим «обрезки углов» кадра (реф.: OCTANGLE_AND_ROUND/OCTANGLE_ONLY/ROUND_ONLY).
    enum CornerCutMode {
        CORNER_OCTANGLE_AND_ROUND = 0,
        CORNER_OCTANGLE_ONLY      = 1,
        CORNER_ROUND_ONLY         = 2,
    };

    // Диапазон [min,max] смещения центра кадра по типу прошивки (1:1 с X2000).
    // Горизонталь: {1,3,5,8}→[-16,16]; 0→[-4,4]; прочие→[0,0].
    static QPair<int, int> GetCenterOffsetHorizontalRange(int fw);
    // Вертикаль:  {1,3}→[-10,10]; {5,8}→[-16,16]; 0→[-4,4]; прочие→[0,0].
    static QPair<int, int> GetCenterOffsetVerticalRange(int fw);

    // Сохранение области отображения в display-ini через KDisplayOption
    // (реф. KVideoCal::SaveDisplayArea → setVideoRectForImgPro + setVideoRectForUI).
    // imgPro — область для обработки изображения ([VIDEO]/IMAGE),
    // ui — область для UI-вывода ([UI]/IMAGE). Возвращает true при записи.
    static bool SaveDisplayArea(const QRect &imgPro, const QRect &ui);

    // ---- device-bound (реализация на приборе, Фаза E) ----
    // SaveCenterPoint  → KEndoScope/KCamera::SetVideoCentorPoint (EEPROM/регистр)
    // SaveCaptureAreaShift → KEndoScope::SetVideoCapArea
    // SaveCornerShape/SaveCornerPara → KEndoScope + GetDefaultVideoCornerCutting
    //   (дефолты из KEncStyle::getScopeDefaultRoundCut/getScopeDefaultOctangleCut).
};
