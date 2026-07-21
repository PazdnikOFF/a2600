#pragma once

#include "ui/KDialog.h"

// Диалог редактора шаблонов отчёта (реф. KReportTempletEditDlg : KFullScreenDialog — НЕ
// KDialog и НЕ data-класс KReportTemplateManager!, ctor @0x51f300, Ui_KReportTempletEditDlg::
// setupUi @0x51f928). UI-порт. Открывается из KReportEditUi::ClickBtnTemplateSettings.
// Полноэкранный 1920×1080 (реф. base KFullScreenDialog(parent,-1), 2-й арг = int-id -1,
// не bool) → портируем над KDialog(FULLSCREEN). SetKStyle НЕТ, титул SetTitle(TR_Tpte).
// Слева (stretch 0:1): label TR_Tpte: + comb_templet (device — библиотека шаблонов/
// отделение) + tree_content (KTempletTreeWidget→QTreeWidget, 1 колонка, header скрыт,
// чекбоксы секций). Справа right_widget (minW 1200): txt_edit_templet (KNewTempletEditor→
// QTextEdit, 820², read-only WYSIWYG-документ). Снизу кнопки Exit/Save&Exit | EditInfo/
// Default. Тексты кнопок переопределяются в InitWidget (Exit→TR_CExit, Default→TR_Dflt).
//
// DEVICE в порт не тянется: comb_templet (KReportTemplateManager::GetTempletsInfos),
// tree_content (KReportTemplateDataNew), txt_edit_templet (KDocumentGenerator→QTextDocument),
// OnSaveAndExit/OnResetDefault/OnDeptChanged/OnEditInfoClicked/add-del-update — заглушки.
// Exit→close.
class KReportTempletEditDlg : public KDialog
{
    Q_OBJECT
public:
    explicit KReportTempletEditDlg(QWidget *parent = nullptr);

private:
    void setupUi();
};
