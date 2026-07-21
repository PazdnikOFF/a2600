#pragma once

#include "ui/KDialog.h"

// Диалог версий ПО/железа (реф. KVersion : KDialog, ctor @0x6ebd90, Ui_KVersion::setupUi
// @0x6ed500). UI-порт. Диалог, SetKStyle(W460), титул TR_Vson. Сетка 19 строк × 3 колонки
// (check-иконка | подпись | значение): product/release/complete/app/kernel/hmi/panel/pap/pas/
// papp00-07(без 05)/papp80/lcd/cam. Снизу — кнопка Exit. Значения тянутся из KVersionConfig
// (off-device читаемо с ENDO_ROOT); check-иконки (versionCheck/yes|no|unknown.png) и ролевая/
// device-видимость строк — device/привилегии (опущены). btn_exit/PanelKeyVersion → close.
class KVersion : public KDialog
{
    Q_OBJECT
public:
    explicit KVersion(QWidget *parent = nullptr);

private:
    void setupUi();
    void showSoftwareVersion();   // реф. ShowSoftwareVersion @0x6e9050: значения из KVersionConfig
};
