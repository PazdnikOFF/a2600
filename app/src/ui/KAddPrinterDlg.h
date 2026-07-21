#pragma once

#include "ui/KDialog.h"

// Диалог добавления принтера (реф. KAddPrinterDlg : KDialog, ctor @0x7972f0,
// Ui_KAddPrinterDlg::setupUi @0x798568). UI-порт. Немодальный, 828×381, SetKStyle НЕТ,
// титул SetTitle(TR_APrinter) + тёмный setStyleSheet. Грид: имя (validator [A-Z_a-z0-9-]
// {1,16}) + тип подключения (combo WPrinter/UPrinter/NPrinter статично) + адрес устройства
// (QStackedWidget: page0 URL-combo device / page1 IP-edit) + Search + драйвер (readonly,
// NoFocus) + AddDriver; ряд чекбоксов (default img/report printer, report checked) +
// OK/Cancel. Кастом KIpLineEdit→QLineEdit+маска.
//
// ConnectType переключает stacked-страницу (реф. OnConnectTypeCmbCurrentIndexChanged) —
// чистый UI. DEVICE: OnOkBtnClicked(SavePrinter/AddUsb/Net/WinPrinter), OnSearchBtnClicked
// (скан подсети), AddUsbPrinterToComBox (CUPS), OnAddDriverBtnClicked (под-диалог) —
// заглушки. Cancel→close.
class KAddPrinterDlg : public KDialog
{
    Q_OBJECT
public:
    explicit KAddPrinterDlg(QWidget *parent = nullptr);

private:
    void setupUi();
};
