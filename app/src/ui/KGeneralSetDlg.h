#pragma once

#include "ui/KDialog.h"

// Диалог общих настроек (реф. KGeneralSetDlg : KDialog, ctor @0x5eada0, Ui_KGeneralSetDlg::
// setupUi @0x5ec1d8). UI-порт. Диалог 400×785, SetKStyle(W460), титул TR_Gnrl4. Чистые layout'ы
// (без абсолютной геометрии). 4 секции: (A) больница — имя/лого/кнопки эндоскопа; (B) поля
// инфо пациента (9 чекбоксов видимости); (C) управление учётками — режим логина + форс-логаут +
// таймаут; (D) кнопки Default/Save/Exit. Сепараторы — реф. KLineH (QFrame HLine; у нас QFrame).
//
// DEVICE в порт не тянется: LoadSystemConf/SaveAccountSet/ImportHospitalLogo/LoadDefault (чтение/
// запись конфига + логотип с USB) — заглушки; connect'ы на слоты опущены (кроме Exit→close).
class KGeneralSetDlg : public KDialog
{
    Q_OBJECT
public:
    explicit KGeneralSetDlg(QWidget *parent = nullptr);

private:
    void setupUi();
};
