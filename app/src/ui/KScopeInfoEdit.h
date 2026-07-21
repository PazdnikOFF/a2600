#pragma once

#include "ui/KDialog.h"

// Диалог редактора информации об эндоскопе (реф. KScopeInfoEdit : KDialog, ctor @0x644cf0,
// Ui_KScopeInfoEdit::setupUi @0x64ad10). UI-порт. Немодальный, 600×919, SetKStyle(W460),
// титул SetTitle(TR_EInfo). Секции: (frame_2) сервис-логин — SN/CN + Login/Logout +
// endoSN + импорт auth; тип (cmb_type device); спека (model/type readonly-edit+device-комбо,
// каналы/длины: 3 Φ-mm double-спина dec1 max25.5 step0.1 + рабочая длина QSpinBox mm
// max5000 step5); статус-виджет (KScopeStaus) + stackedWidget (камера/скоп); (frame_manu)
// произв. инфо — дата(1970-2050)/ESN/частота (readonly); (frame_user) польз. инфо —
// contrastno(maxLen16)/comment(maxLen20); Save/Exit.
//
// Кастом заменены: KLineH×3→QFrame(HLine); KScopeStaus→плейсхолдер-фрейм.
//
// DEVICE в порт не тянется: GetEndoScope (ShowEndoInfoCID/EepromSaveRet, OnSave→EEPROM/CID),
// ClickImportAuthBin, ClickBtnPUserLogin/Logout, LoadScopeInfo (комбо), SaveInfoTimeOut —
// заглушки. ValueChanged (dirty-трек)/Exit→close — чистый UI.
class KScopeInfoEdit : public KDialog
{
    Q_OBJECT
public:
    explicit KScopeInfoEdit(QWidget *parent = nullptr);

private:
    void setupUi();
};
