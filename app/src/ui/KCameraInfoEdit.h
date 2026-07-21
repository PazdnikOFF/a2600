#pragma once

#include "ui/KDialog.h"

// Диалог редактора информации о камере (реф. KCameraInfoEdit : KDialog, ctor @0x5fdf58,
// Ui_KCameraInfoEdit::setupUi @0x6033e0). UI-порт. Камерный близнец KScopeInfoEdit
// (cmb_type в KScopeInfoEdit переключает на него). Немодальный, 600×843, SetKStyle(W460),
// титул SetTitle(TR_CHInfo). Секции: frame_2 сервис-логин (SN/CN device-метки + Login/
// Logout + CHSN + import auth); тип (cmb_type статично SLens/HLens); сетка model
// (readonly-edit + device-комбо) + esn (maxLen10, валидатор [A-Za-z0-9]{0,10});
// frame_manu — пустой плейсхолдер; Save/Exit. Все stock Qt, кастомов нет.
//
// ПРИМ: в реф. большинство service-виджетов скрыты по умолчанию (setVisible(false),
// гейт по KAccount::CurrentRole — device); у нас показаны (роль-гейтинг заглушён).
//
// DEVICE в порт не тянется: GetCamera(ShowCameraInfoSaveRet/OnSave→EEPROM),
// ClickImportAuthBin/UserLogin-Logout, OpenEndoInfoEdit, SN/CN из KSystemSet,
// cmb_model(LoadEndoModel), роль-гейтинг — заглушки. ValueChanged/Exit→close — UI.
class KCameraInfoEdit : public KDialog
{
    Q_OBJECT
public:
    explicit KCameraInfoEdit(QWidget *parent = nullptr);

private:
    void setupUi();
};
