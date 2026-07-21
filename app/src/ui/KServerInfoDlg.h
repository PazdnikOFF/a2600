#pragma once

#include "ui/KDialog.h"

// Диалог настроек сервера (реф. KServerInfoDlg : KDialog, ctor @0x5efe60,
// Ui_KServerInfoDlg::setupUi @0x5f0a30). UI-порт. Немодальный, 562×587, SetKStyle(W460).
// Открывается из KAuthMachineDlg::SetServerInfo. Несмотря на имя — это конфиг DNS/прокси:
// группа TR_SSettings2 с полями DNS1/DNS2 + прокси/базовый URL (label_proxy) + Save/Exit.
// Всё stock Qt (QLineEdit без maxLength/validator), кастомов нет.
//
// DEVICE в порт не тянется: InitWidgets (чтение endoinfoserver.ini → поля), Save (запись
// ini: dns1/dns2/proxy/loginurl/endoinfoposturl + перезапись /etc/resolv.conf) — заглушки.
// Exit→close.
class KServerInfoDlg : public KDialog
{
    Q_OBJECT
public:
    explicit KServerInfoDlg(QWidget *parent = nullptr);

private:
    void setupUi();
};
