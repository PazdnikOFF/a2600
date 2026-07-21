#pragma once

#include "ui/KDialog.h"

// Диалог предпросмотра печати отчёта (реф. KReportPreviewDlg : KFullScreenDialog — НЕ
// KDialog!, ctor @0x500d08, Ui_KReportPreviewDlg::setupUi @0x503108). UI-порт.
// Открывается из KReportEditUi::ClickBtnPrintPreveiw. Полноэкранный 1920×1080 (реф. base
// KFullScreenDialog(parent,-1), 2-й арг = int-id окна, не bool); портируем над
// KDialog(FULLSCREEN). SetKStyle НЕТ, титул SetTitle(TR_Prvw2).
// Слева тулбар (235:1685): по ширине/по странице/печать | линия | zoom+/zoom- | масштаб
// (comboBox_scale editable, 29 % + regex-валидатор) | шаблон (device) | Exit.
// Справа область предпросмотра (widget_preview/gridLayout_preview) — реф. встроенный
// KReportPreviewCenterDlg (рендер страницы) → плейсхолдер QLabel.
//
// DEVICE в порт не тянется: OnBtnPrint (QPrinter), comboBox_template/OnDeptChanged/
// RefreshComboBox (список отделений/шаблонов), весь рендер-пайплайн центральной вьюхи —
// заглушки. Zoom+/− двигают comboBox_scale (чистый UI, реализовано). Exit→close.
class KReportPreviewDlg : public KDialog
{
    Q_OBJECT
public:
    explicit KReportPreviewDlg(QWidget *parent = nullptr);

private:
    void setupUi();
};
