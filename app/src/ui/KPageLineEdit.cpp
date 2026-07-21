#include "KPageLineEdit.h"

#include <QIntValidator>

KPageLineEdit::KPageLineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    // Реф. ctor @0x7b6990: m_value=1, валидатор из GetRegFromNumber(1) ([1,1]).
    setValidator(new QIntValidator(1, 1, this));
}

void KPageLineEdit::SetMaximumValue(int max)
{
    // Реф. @0x7b6b20: пересобрать валидатор под [1,max]. max в члене НЕ хранится (только форма
    // валидатора), как в реф.
    if (max < 1)
        max = 1;
    setValidator(new QIntValidator(1, max, this));
}

void KPageLineEdit::setText(const QString &text)
{
    // Реф. @0x7b57e0: закоммитить значение в m_value, затем база.
    m_value = text.trimmed().toInt();
    QLineEdit::setText(text);
}

void KPageLineEdit::focusOutEvent(QFocusEvent *e)
{
    // Реф. @0x7b6c68: пустое ИЛИ расхождение с m_value → вернуть отображение к m_value.
    if (text().isEmpty() || text().trimmed().toInt() != m_value)
        QLineEdit::setText(QString::number(m_value));
    QLineEdit::focusOutEvent(e);
}
