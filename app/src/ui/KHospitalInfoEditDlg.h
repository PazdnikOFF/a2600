#pragma once

#include "ui/KDialog.h"

// Диалог редактирования инфо больницы/предприятия (реф. KHospitalInfoEditDlg : KDialog,
// ctor @0x559790, Ui_KHospitalInfoEditDlg::setupUi @0x5599d0). UI-порт. Немодальный,
// 674×540, SetKStyle НЕТ, титул SetTitle(TR_EHInformation) (перекрывает setupUi TR_HIEdit).
// Форма-грид: логотип (label_logo_img, белый фон, pixmap device) + caption1 (имя больницы,
// config) + caption2 (KMemComboBox→editable QComboBox, maxLen30) + textEdit_statement
// (декларация) + ряд Default/Save/Cancel (фикс. 156×42). Все stock Qt кроме KMemComboBox.
//
// DEVICE в порт не тянется: логотип (KSystemSet::GetHospitaLogo), label_cap1_val/statement/
// combo-items из report-template config (LoadDataFromReportTemplateConfig/InitQuickInput),
// OnBtnSaveClicked (persist) — заглушки. Cancel→close.
class KHospitalInfoEditDlg : public KDialog
{
    Q_OBJECT
public:
    explicit KHospitalInfoEditDlg(QWidget *parent = nullptr);

private:
    void setupUi();
};
