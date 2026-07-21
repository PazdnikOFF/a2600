#pragma once

#include "ui/KDialog.h"

class QLabel;
class QLineEdit;
class QPushButton;

// Диалог входа (реф. KLogin : KDialog, ctor @0x6f78d0, Ui_KLogin::setupUi @0x6f7d30). UI-порт.
// Диалог 460×1080 (портрет), SetKStyle(W460), титул TR_Lgn, БЕЗ close-кнопки. Форма: аккаунт
// (lineEdit_id, maxLen 8) + пароль (lineEdit_passwd, echoMode Password, maxLen 16); кнопки
// Login/Logout (logout скрыт, если не IsAutoLogin). Верхний margin 360 опускает форму вниз
// высокого окна. setStyleSheet нет (глобальный theme qss).
//
// DEVICE в порт не тянется: проверка пароля (Login/KAccount), InitAccount, IsAutoLogin,
// password-regex-валидатор — заглушки.
class KLogin : public KDialog
{
    Q_OBJECT
public:
    explicit KLogin(QWidget *parent = nullptr);

private slots:
    void Login();    // реф.: проверка учётки (device)
    void Logout();   // реф.: выход (device)

private:
    void setupUi();

    QLabel      *label_hints = nullptr;
    QLineEdit   *lineEdit_id = nullptr;
    QLineEdit   *lineEdit_passwd = nullptr;
    QPushButton *btn_login = nullptr;
    QPushButton *btn_logout = nullptr;
};
