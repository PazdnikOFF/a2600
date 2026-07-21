#pragma once

#include "ui/KDialog.h"

// Диалог добавления/редактирования DICOM-сервиса (реф. KDICOMServiceEditDlg : KDialog,
// ctor @0x5b1a18, Ui_KDICOMServiceEditDlg::setupUi @0x5b2700). UI-порт. Немодальный,
// 639×568, SetKStyle(W700). Открывается из KSysDicom (Add/Edit).
// Форма-грид: тип сервиса (Storage/Commitment/Worklist/MPPS) + имя + режим адреса
// (IP/домен через cmb_server → QStackedWidget) + Ping + порт + Echo(верификация) +
// AE-title + тип commitment + макс. результатов + описание; снизу подсказка
// «* обязательное поле» + кнопки Confirm/Cancel/Reset.
// Реф. title в ctor: cur==null → TR_ADService (add), иначе TR_EDService (edit) —
// у нас add-режим.
//
// Кастом KIpAddrEdit → QLineEdit c IP-маской. cmb_server (TR_SIP:/TR_SDName)
// переключает IP↔домен (SlotToChangeInputServerType) — чистый UI.
//
// DEVICE в порт не тянется: Ping/EchoService (ICMP/C-ECHO + таймеры), Save/
// SaveAdd/SaveEditService/IsExistSameSerice (persist), cmb_cmtType из KDICOMLocalConf,
// LoadData — заглушки. Confirm→Save(device, не подключён), Cancel→close, Reset→UI-стуб.
class KDICOMServiceEditDlg : public KDialog
{
    Q_OBJECT
public:
    explicit KDICOMServiceEditDlg(QWidget *parent = nullptr);

private:
    void setupUi();
};
