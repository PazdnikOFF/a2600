#pragma once

#include "ui/KDialog.h"

// Диалог поиска сетевых принтеров (реф. KNetPrintList : KDialog, ctor @0x78cc90,
// Ui_KNetPrintList::setupUi @0x78d160). UI-порт. Немодальный, 646×490, SetKStyle НЕТ,
// титул SetTitle(TR_Sch) + тёмный setStyleSheet. frame → gridLayout_2 (label_msg TR_Schng.
// «поиск…» + m_pfindIpList QListWidget, device-скан) + ряд кнопок Search/OK/Cancel
// (все стартуют disabled, включаются по device-логике/выбору). Все stock Qt, кастомов нет.
//
// DEVICE в порт не тянется: QTimer/StartSearch/LoadPrinterList/RefreshList (скан через
// KHalPrinterAPI), ClickBtnOK (commit принтера), time2RefreshPrinter, SubscribeMsg(0x2b2b)/
// HandleSubscribeMsg — заглушки; список пуст. Cancel→close (чистый UI).
class KNetPrintList : public KDialog
{
    Q_OBJECT
public:
    explicit KNetPrintList(QWidget *parent = nullptr);

private:
    void setupUi();
};
