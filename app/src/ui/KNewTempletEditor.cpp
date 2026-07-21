#include "KNewTempletEditor.h"

KNewTempletEditor::KNewTempletEditor(QWidget *parent)
    : QTextEdit(parent)
{
    // Реф. ctor @0x55cf30: минимальный — setReadOnly(true).
    setReadOnly(true);
}

void KNewTempletEditor::SetPreviewHtml(const QString &html)
{
    // Порт-стаб: реф. контент рендерится KDocumentGenerator (device); здесь — статичный html.
    setHtml(html);
}
