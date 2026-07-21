#pragma once

#include "ui/KDialog.h"

// Диалог сохранения фразы в глоссарий (реф. KThesaurusSaveUi : KDialog, ctor @0x4e3e30,
// Ui_KThesaurusSaveUi::setupUi @0x4e4160). UI-порт. Немодальный, фикс. 1024×768,
// SetKStyle НЕ вызывается; титул SetTitle(TR_AGlossary). Открывается из KReportEditUi
// (btn_save_as_word_bank). Абсолютная геометрия (без layout'ов). Два блока:
//   • группа/заголовок: cmb_group (editable, device-список) + edit_title (maxLen 100) +
//     подписи TR_Grp:/TR_Ttle: с красными «*»;
//   • находки/заключение: edit_examfinding (QTextEdit, префилл) + edit_diagresult
//     (QTextEdit, префилл) под TR_VOExam:/TR_EConclusion:.
// Снизу подсказка TR_RField + Save/Cancel. Все виджеты — stock Qt.
//
// DEVICE в порт не тянется: KThesaurusOpt (ReadFile/GetDiseagroupList → cmb_group;
// AddDiseaseContent на Save; валидация непустоты + KMessageBox::warning) — заглушки.
// Cancel→close.
class KThesaurusSaveUi : public KDialog
{
    Q_OBJECT
public:
    explicit KThesaurusSaveUi(QWidget *parent = nullptr);

private:
    void setupUi();
};
