#pragma once

#include "ui/KDialog.h"

// Диалог настройки полей отчёта (реф. ReportConfigDlg : KDialog — имя БЕЗ K-префикса!,
// ctor @0x4f2da8, Ui_ReportConfigDlg::setupUi @0x4f6e10). UI-порт. Немодальный, 1085×757,
// SetKStyle НЕТ, титул SetTitle(TR_Set). Верх: тип обследования (combo TR_Exm 1/2/OMode3)
// + метка TR_Used (cyan). Центр (widget_center, тёмная скруглённая панель): 3 секции
// чекбокс+метка полей 4 колонки — TR_PInfo (14 полей ENo/EmDate/EInfo/Stts/Nme/Gdr/Age/
// PID/Aplct/DoB/Tel/BNo/2польз), TR_Dgnse (6: VOExam/EConclusion/DName/OMode3/IFindings/
// Sgstn + 2 польз-поля с lineEdit), прочее (BSite/HP/ADoctor/Dctr) + подсказка
// TR_VITREWChecked. Снизу Save/Cancel. Всё stock Qt, кастомов и DEVICE-слотов НЕТ.
//
// Конфиг-модель KReportEditUIConfig (combo-перезагрузка полей / Save-запись) — заглушка.
// Cancel→close.
class ReportConfigDlg : public KDialog
{
    Q_OBJECT
public:
    explicit ReportConfigDlg(QWidget *parent = nullptr);

private:
    void setupUi();
};
