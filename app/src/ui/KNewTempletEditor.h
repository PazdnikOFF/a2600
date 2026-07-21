#pragma once

#include <QTextEdit>
#include <QString>

// Read-only превью отчёта/шаблона (реф. KNewTempletEditor @ctor 0x55cf30, base QTextEdit).
// UI-порт. ОТЛИЧАЕТСЯ от KTempletTreeWidget (тот QTreeWidget) — это rich-text превью.
// ctor: setReadOnly(true). Контент реф. рендерится из KReportTemplateItem через
// KDocumentGenerator (DEVICE) — в порте оболочка read-only + SetPreviewHtml (стаб).
class KNewTempletEditor : public QTextEdit
{
    Q_OBJECT
public:
    explicit KNewTempletEditor(QWidget *parent = nullptr);

    // Порт-стаб контента (реф. рендер через KDocumentGenerator::ClickSubItem — device).
    void SetPreviewHtml(const QString &html);

private:
    QString m_selectedRefId;   // +0x48 (реф. ref-id выбранного item)
};
