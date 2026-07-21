#pragma once

#include "ui/KDialog.h"

// Диалог настройки списка обследований (реф. KExamListSetupDlg : KDialog, ctor @0x7f97e0,
// Ui_KExamListSetupDlg::setupUi @0x7fb970). UI-порт. Немодальный, ФИКС 1024×712,
// SetKStyle(W1024), титул SetTitle(TR_MRLSettings). Абсолютная геометрия (без layout'ов).
// Верх grp_list: label TR_LDisplay + 12 чекбоксов видимости колонок списка (пол/возраст/
// дата/направитель/эндоскоп/SN/ДР/тел/койка/№рег/2 польз-поля) + подсказка. Низ grp_path:
// label TR_EPath + 2 радио пути экспорта на USB (ExportUdiskPath1/2, эксклюзив). Снизу
// Save/Exit. Все виджеты — stock Qt.
//
// DEVICE в порт не тянется: SaveSetupData (KExamListConfigHandler: SetExportPath/SetIsShow*),
// LoadConfigData/ResetWidgetLayout (чтение флагов) — заглушки; чек-состояния пусты.
// Exit→close.
class KExamListSetupDlg : public KDialog
{
    Q_OBJECT
public:
    explicit KExamListSetupDlg(QWidget *parent = nullptr);

private:
    void setupUi();
};
