#include "KCounterTextEdit.h"

#include <QTextCursor>

KCounterTextEdit::KCounterTextEdit(QWidget *parent)
    : QTextEdit(parent)
{
    // Реф. ctor @0x5a7d10: базовый QTextEdit, m_nMaxLength=800. textChanged НЕ подключён —
    // счётчик/обрезка активируются только в InitWidget.
}

void KCounterTextEdit::InitWidget(int nMaxLength)
{
    // Реф. @0x5a7d50: n<0 → no-op (без ограничения); иначе задать cap и подключить счётчик.
    if (nMaxLength < 0)
        return;
    m_nMaxLength = nMaxLength;
    connect(this, &QTextEdit::textChanged, this, &KCounterTextEdit::OnTextChanged);
}

QString KCounterTextEdit::GetCounterShowText(int nCount) const
{
    // Реф. @0x5a7df8: "nCount/max" (счёт ВВЕРХ, не остаток).
    return QString::number(nCount) + QStringLiteral("/") + QString::number(m_nMaxLength);
}

void KCounterTextEdit::OnTextChanged()
{
    // Реф. @0x5a8048: обрезка по QChar + курсор в конец + эмит счётчика.
    const QString t = toPlainText();
    if (m_nMaxLength < t.length()) {
        setPlainText(t.left(m_nMaxLength));   // повторно триггерит textChanged, но уже == max → else
        QTextCursor c = textCursor();
        c.movePosition(QTextCursor::End);
        setTextCursor(c);
        emit ChangeCounterShowText(GetCounterShowText(m_nMaxLength));
    } else {
        emit ChangeCounterShowText(GetCounterShowText(t.length()));
    }
}
