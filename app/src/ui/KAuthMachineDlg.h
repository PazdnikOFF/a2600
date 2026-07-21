#pragma once

#include "ui/KDialog.h"

// Диалог авторизации машины (реф. KAuthMachineDlg : KDialog, ctor @0x5d5ba0,
// Ui_KAuthMachineDlg::setupUi @0x5d8c28). UI-порт. SetKStyle(W460).
// ВАЖНО: диалог только отображает — поля ввода кода авторизации НЕТ. Показывает
// SN процессора / CN изделия / срок действия / остаток и две кнопки-действия
// (авторизовать машину, настройки сервера) + Exit. Одна группа TR_TMAuthorization,
// внутри 4 строки «подпись : значение» + блок из 2 кнопок (minW 420) + Exit (fixed 160).
//
// DEVICE в порт не тянется: ClickAuthMachine (DES/KControlProc — авторизация),
// SetServerInfo, и заполнение значений из KSystemSet (GetProductCN/GetProcessorSN/
// GetProductAuthFlag/GetLastValidDate/GetRemainDays) — заглушки; поля-значения пусты.
// Exit→close.
class KAuthMachineDlg : public KDialog
{
    Q_OBJECT
public:
    explicit KAuthMachineDlg(QWidget *parent = nullptr);

private:
    void setupUi();

    int m_authState = -1;  // реф. this+0x58, init -1
};
