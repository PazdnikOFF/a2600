#pragma once

#include "ui/KDialog.h"

#include <QString>

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

    // Реф. ctor принимает (QString, QString, KScopeClass::E_CLASS, QWidget*) — тексты
    // сохраняемой статьи и класс скопа. У нас ctor исторически без них, поэтому контекст
    // задаётся сеттером (зовёт OpenThesaurusSaveDlg @0x4e3f40 перед DoModal).
    void SetSaveContext(const QString &text, const QString &title, int scopeClass);
    QString SaveText() const { return m_text; }
    QString SaveTitle() const { return m_title; }
    int ScopeClass() const { return m_scopeClass; }

private:
    QString m_text, m_title;
    int m_scopeClass = 0;
    void setupUi();
};
