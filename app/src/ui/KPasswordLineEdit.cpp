#include "KPasswordLineEdit.h"

#include <QRegExpValidator>

KPasswordLineEdit::KPasswordLineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    // Реф. ctor @0x690200: только connect textChanged→OnTextChanged. echoMode НЕ ставится.
    connect(this, &QLineEdit::textChanged, this, &KPasswordLineEdit::OnTextChanged);
}

void KPasswordLineEdit::setValidator(const QValidator *v)
{
    // Реф. @0x690318: null → no-op. Странный guard реф. — проверяет валидность ТЕКУЩЕГО
    // m_regexp (дефолтный пустой QRegExp валиден) перед присваиванием; воспроизводим точно.
    if (!v || !m_regexp.isValid())
        return;
    if (const QRegExpValidator *rv = qobject_cast<const QRegExpValidator *>(v))
        m_regexp = rv->regExp();   // берём regexp, сам QValidator отбрасываем
}

void KPasswordLineEdit::OnTextChanged(const QString &s)
{
    // Реф. @0x690368: обрезать хвост, пока не exactMatch; переписать при изменении.
    if (!m_regexp.isValid())
        return;
    QString t = s;
    while (!t.isEmpty() && !m_regexp.exactMatch(t))
        t.chop(1);
    if (t != s)
        QLineEdit::setText(t);
}
