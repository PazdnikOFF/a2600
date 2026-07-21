#pragma once

#include "ui/KDialog.h"

// Диалог настроек принтеров (реф. KSysPrinter : KDialog, ctor @0x79c3d8,
// Initialize @0x79c0e0, Ui_KSysPrinter::setupUi @0x79c850). UI-порт. Это сиблинг
// KSystemSetDlg (кнопка TR_PSET2/F3 → OpenSystemSetDlg код 23).
// Реф. без SetKStyle/SetTitle: frameless + тема-QSS + setGeometry pos(230,160) 1680×900.
// Состав: заголовок TR_SPrinter (центр, крупный) + QTableWidget на 5 колонок
// (TR_PName/TR_SType/TR_CType2/TR_Dvc/TR_Dflt1, строки — device) + ряд кнопок
// Add/Del/Default/PrintSettings + Exit (реф. абсолютно снизу-справа).
// Только stock-виджеты, кастом-типов нет.
//
// DEVICE в порт не тянется: KPrinterManager (список принтеров, RefreshTable),
// OnAdd/OnDel/OnDefault/OnPrintSettings (CUPS/config I/O), getKDialogThemeQss —
// заглушки; таблица пуста. Del/Default/PrintSettings активны только при выделении
// строки (реф. OnTableWidgetSelectionChanged). Exit→close.
class KSysPrinter : public KDialog
{
    Q_OBJECT
public:
    explicit KSysPrinter(QWidget *parent = nullptr);

private:
    void setupUi();
};
