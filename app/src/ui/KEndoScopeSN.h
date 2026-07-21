#pragma once

#include "ui/KDialog.h"

// Диалог ввода серийника эндоскопа (реф. KEndoScopeSN : KDialog, ctor @0x742110,
// Ui_KEndoScopeSN::setupUi @0x7428a8). UI-порт. Немодальный, 640×480, SetKStyle(W460),
// титул TR_ESN. Центрирующая сетка со спейсерами: заголовок TR_PETESNNumber: + строка
// SN:/lineEdit (validator [A-Za-z0-9]{0,12}) + ряд OK/Cancel (фикс 100). Все stock Qt.
//
// DEVICE в порт не тянется: onOK (валидация SN >8 симв. + KMessageBox::warning + запись
// в KEndoScope) — заглушка (пустой-чек оставлен, запись опущена). Cancel→close.
class KEndoScopeSN : public KDialog
{
    Q_OBJECT
public:
    explicit KEndoScopeSN(QWidget *parent = nullptr);

private:
    void setupUi();
};
