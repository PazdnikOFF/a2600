#pragma once

#include "ui/KDialog.h"

// Диалог смены пароля (реф. KPasswordEdit : KDialog, ctor @0x456048,
// Ui_KPasswordEdit::setupUi @0x456ec0). UI-порт. Немодальный, 320×240, SetKStyle(W320),
// титул SetTitle(TR_PEdit). Сетка: новый пароль (KPasswordLineEdit→QLineEdit, echo
// Password, maxLen16) + подтверждение (то же) + ряд кнопок OK(QToolButton, выключен пока
// оба поля не заполнены)/Cancel.
//
// Кастом KPasswordLineEdit→QLineEdit(Password). OnCheckInput (вкл. OK при непустых) +
// IsPasswordConfirmSame (сверка) — чистый UI, реализованы. Cancel→close.
//
// DEVICE в порт не тянется: GetKAccount (GetPasswordRegExp/ValidateIfPWValid/
// ConvertPasswordToMD5/GetAdmin), KSystemSet::SaveAccount (Save/SaveData) — заглушки.
class KPasswordEdit : public KDialog
{
    Q_OBJECT
public:
    explicit KPasswordEdit(QWidget *parent = nullptr);

private:
    void setupUi();
};
