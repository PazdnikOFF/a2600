#pragma once

#include "ui/KDialog.h"

// Диалог выбора драйвера принтера (реф. KAddPrinterDriverDlg : KDialog, ctor @0x792618,
// Ui_KAddPrinterDriverDlg::setupUi @0x7931b0). UI-порт. Открывается из KAddPrinterDlg
// (AddDriver). Немодальный, 1044×636, SetKStyle НЕТ, титул SetTitle(TR_ADriver) + тёмный
// setStyleSheet. Строка поиска (TR_DSearch + ledit maxLen128, h40) + две колонки-списка
// (m_pMakerListWidget TR_Mnfctrr 400×400 device / m_pPrinterListWidget TR_PDriver 600×400
// device) + ряд OK/Install(minW180)/Cancel. Все stock Qt, кастомов нет.
//
// DEVICE в порт не тянется: Initialize/OnMakerListWidgetCurrentRowChanged (KPrinterManager
// map производитель→PPD, CUPS-перечисление), OnInstallBtnClicked (установка PPD с диска),
// OnDriverSearchLeditTextChanged (фильтр) — заглушки; списки пусты, OK disabled. OK/Cancel→close.
class KAddPrinterDriverDlg : public KDialog
{
    Q_OBJECT
public:
    explicit KAddPrinterDriverDlg(QWidget *parent = nullptr);

private:
    void setupUi();
};
