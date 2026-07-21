#include "KDateEdit.h"

#include <QLineEdit>

KDateEdit::KDateEdit(QWidget *parent)
    : QDateEdit(parent)
{
    // Реф. ctor @0x813600: плейсхолдер-маска через specialValueText, дата на minimumDate.
    setSpecialValueText(m_placeholder);
    QDateTimeEdit::setDate(minimumDate());   // пустое: дата на минимуме → показ маски
    connect(this, &QDateEdit::dateChanged, this, &KDateEdit::DateChangedSlot);
}

QDate KDateEdit::date() const
{
    // Реф. @0x8139c8: маска активна → InvalidDate.
    if (isEmpty())
        return InvalidDate();
    return QDateEdit::date();
}

void KDateEdit::setDate(const QDate &d)
{
    // Реф. @0x813ca8.
    if (d == InvalidDate()) {
        QDateTimeEdit::clear();
        setSpecialValueText(m_placeholder);
        lineEdit()->setText(m_placeholder);
        return;
    }
    setSpecialValueText(QString());
    QDateTimeEdit::setDate(d);
}

void KDateEdit::clear()
{
    // Реф. @0x813c68.
    QDateTimeEdit::clear();
    setSpecialValueText(m_placeholder);
    lineEdit()->setText(m_placeholder);
}

void KDateEdit::setDisplayFormat(const QString &f)
{
    // Реф. @0x813728: сохранить toggle-состояние плейсхолдера при смене формата.
    const bool wasEmpty = isEmpty();
    QDateTimeEdit::setDisplayFormat(f);
    if (wasEmpty) {
        setSpecialValueText(m_placeholder);
        QDateTimeEdit::setDate(minimumDate());
    }
}

void KDateEdit::focusOutEvent(QFocusEvent *e)
{
    // Реф. @0x813f88: если правился, но не закоммичено → назад в бланк.
    if (!isReadOnly() && m_edited)
        clear();
    QDateEdit::focusOutEvent(e);
}

void KDateEdit::DateChangedSlot(const QDate &d)
{
    // Реф. @0x813a98: не-null дата закоммичена → сброс edit-флага.
    if (d.isValid() && d != InvalidDate())
        m_edited = false;
}
