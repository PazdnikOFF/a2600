#pragma once

#include <QRect>
#include <QPair>
#include <QStringList>

#include "ui/KDialog.h"

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QSpinBox;

// Диалог калибровки видео (реф. класс KVideoCal, X-2600) — UI И логика в ОДНОМ классе,
// как в оригинале. Коллизия ROADMAP C6 разрешена СОВМЕЩЕНИЕМ (как с KDocumentGenerator):
// прежний off-device data-класс был набором статических методов, они сохранены без
// изменений, а сверху надстроен реф.-диалог.
//
// Реф. ctor @0x63b4e8: KDialog(parent, false) → SetKStyle(2) → Ui_KVideoCal::setupUi
// @0x63ca98 → InitWidget → ~30 connect → InitVideoParam; плюс KSystemStatus::SetIsVideoCal(true).
// resize 300×982, objectName "KVideoCal". Все виджеты — стоковые Qt.
//
// Разметка (сверена дизасмом setupUi/retranslateUi): 6 QGroupBox + нижний QFrame:
//   TR_CShape (форма угла), TR_COffset (смещение центра), TR_CArea (область захвата),
//   TR_EType (тип эндоскопа/стекла), TR_DSize (области Video/UI), «图像参数» (AGC/AEC,
//   заголовок — ХАРДКОД-китайский, не TR_-ключ), frame: TR_Cntrd / TR_Ext.
// Подписи «x/y/w/h», «UI», «AGC:», «AEC:», «0-0» — тоже хардкод (не проходят retranslateUi).
//
// DEVICE-STUB: динамические комбобоксы (cmb_corner_mode ← KUserSet::GetCornerModeList,
// cmb_endoType ← KEncStyle::getSupportedScopeList) и все записи в железо
// (KEndoScope/KCamera::SetVideoCentorPoint/SetVideoCapArea) заменены инъектируемыми
// списками и no-op-слотами. Статические расчётные методы ниже — настоящие, off-device.
class KVideoCal : public KDialog
{
    Q_OBJECT
public:
    explicit KVideoCal(QWidget *parent = nullptr);

    // DEVICE-STUB инъекция содержимого динамических комбобоксов (реф. берёт их из
    // KUserSet/KEncStyle — device-состояние, констант в бинарнике нет).
    void SetCornerModeList(const QStringList &modes);
    void SetEndoTypeList(const QStringList &types);

private slots:
    // Реф. слоты (имена 1:1). Тела, пишущие в железо, — заглушки; расчётные части реальные.
    void SwitchCornerShape(int index);          // cmb_corner_type
    void SwitchCornerMode(int index);           // cmb_corner_mode
    void CornerShapeValueXChanged(int v);       // spin_corner_para
    void CornerShapeValueYChanged(int v);       // spin_corner_para_2
    void FlagPosChanged(double v);              // dspin_flag_pos
    void CenterPointValueChanged(int v);        // spin_center_x/_y (общий)
    void CaptureAreaValueChanged(int v);        // spin_cap_x/_y (общий)
    void DisplayAreaValueChanged(int v);        // все 8 spin_display*
    void AGCValueChanged(int v);
    void AECValueChanged(int v);
    void EndoGlassTypeChanged(int index);
    void SaveCornerShape();
    void SaveCenterPoint();
    void SaveCaptureAreaShift();
    void SaveEndoType();
    void SaveEndoglassType();
    void SetDisplayPosition();                  // btn_position
    void ExitAdjustMode();                      // btn_exit

private:
    void setupUi();            // реф. Ui_KVideoCal::setupUi @0x63ca98
    void InitWidget();         // реф. @0x63ad68 (device-части — заглушки)
    void InitEndoGlassType();  // реф. @0x63aad0: G/S/SR/LSR (ASCII, НЕ переводятся)
    void SaveDisplayAreaSlot();   // реф. слот SaveDisplayArea() — тёзка статического метода

    QComboBox      *cmb_corner_type = nullptr;
    QComboBox      *cmb_corner_mode = nullptr;
    QSpinBox       *spin_corner_para = nullptr;
    QSpinBox       *spin_corner_para_2 = nullptr;
    QDoubleSpinBox *dspin_flag_pos = nullptr;
    QSpinBox       *spin_center_x = nullptr;
    QSpinBox       *spin_center_y = nullptr;
    QSpinBox       *spin_cap_x = nullptr;
    QSpinBox       *spin_cap_y = nullptr;
    QComboBox      *cmb_endoType = nullptr;
    QComboBox      *cmb_glasstype = nullptr;
    QSpinBox       *spin_displayv_x = nullptr;
    QSpinBox       *spin_displayv_y = nullptr;
    QSpinBox       *spin_displayv_w = nullptr;
    QSpinBox       *spin_displayv_h = nullptr;
    QSpinBox       *spin_displayu_x = nullptr;
    QSpinBox       *spin_displayu_y = nullptr;
    QSpinBox       *spin_displayu_w = nullptr;
    QSpinBox       *spin_displayu_h = nullptr;
    QSpinBox       *spin_agc = nullptr;
    QSpinBox       *spin_aec = nullptr;
    QLabel         *label_agc_range = nullptr;
    QLabel         *label_aec_range = nullptr;
    QPushButton    *btn_position = nullptr;
    QPushButton    *btn_exit = nullptr;

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
