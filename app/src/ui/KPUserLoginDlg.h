#pragma once

#include "ui/KDialog.h"

// Диалог входа привилегированного/сервисного пользователя (реф. KPUserLoginDlg : KDialog,
// ctor @0x5de170, Ui_KPUserLoginDlg::setupUi @0x5dfba0). UI-порт. Немодальный, 600×400,
// SetKStyle(W460). groupBox(TR_DLogin) с центрированной формой: account(TR_Acnt:, maxLen512)
// + password(TR_Pswd:, KPasswordLineEdit→QLineEdit echo Password, minW180, maxLen512,
// blacklist-валидатор) + ряд Login/Cancel(=Logout, 160-широкие). Все stock Qt кроме
// KPasswordLineEdit. HideAutoLogin() прячет пароль (авто-логин-вариант).
//
// DEVICE в порт не тянется: Login() (проверка учётки по БД) — заглушка. CancelLogin→close,
// eventFilter (Enter→Login) — чистый UI.
class KPUserLoginDlg : public KDialog
{
    Q_OBJECT
public:
    explicit KPUserLoginDlg(QWidget *parent = nullptr);

private:
    void setupUi();
};
