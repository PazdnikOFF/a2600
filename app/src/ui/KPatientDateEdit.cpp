#include "KPatientDateEdit.h"

#include <QLineEdit>
#include <QKeyEvent>

KPatientDateEdit::KPatientDateEdit(QWidget *parent)
    : QDateEdit(parent)
{
    // Реф. ctor @0x7b7e88: плейсхолдер = m_format через specialValueText; коннекты цвета.
    setSpecialValueText(m_format);
    QDateTimeEdit::setDate(minimumDate());   // пустое состояние: дата на минимуме → показ specialValueText
    connect(this, &QDateEdit::dateChanged, this, &KPatientDateEdit::DateChangedSlot);
    connect(lineEdit(), &QLineEdit::textChanged, this, &KPatientDateEdit::SlotToChangeTextColor);
    applyTextColor();
}

bool KPatientDateEdit::isEmpty() const
{
    // Пусто = сейчас показывается плейсхолдер (specialValueText == m_format).
    return specialValueText() == m_format;
}

QDate KPatientDateEdit::date() const
{
    // Реф. @0x7b8160: пусто → сентинел InvalidDate.
    if (isEmpty())
        return InvalidDate();
    return QDateEdit::date();
}

void KPatientDateEdit::setDate(const QDate &d)
{
    // Реф. @0x7b85e0.
    if (d == InvalidDate()) {
        QDateTimeEdit::clear();
        setSpecialValueText(m_format);
        lineEdit()->setText(m_format);
        applyTextColor();
        return;
    }
    setSpecialValueText(QString());
    QDateTimeEdit::setDate(d);
    emit dateChanged(d);
    applyTextColor();
}

void KPatientDateEdit::setDisplayFormat(const QString &f)
{
    // Реф. @0x7b8018: m_format = upper (плейсхолдер), при пустом переприменить.
    // ВАЖНО: emptiness проверять ДО смены m_format (isEmpty сравнивает specialValueText с m_format).
    const bool wasEmpty = isEmpty();
    m_format = f.toUpper();
    QDateTimeEdit::setDisplayFormat(f);
    if (wasEmpty) {
        setSpecialValueText(m_format);
        QDateTimeEdit::setDate(minimumDate());   // держать на минимуме → показ нового плейсхолдера
    }
}

void KPatientDateEdit::clear()
{
    // Реф. @0x7b85a0.
    QDateTimeEdit::clear();
    setSpecialValueText(m_format);
    lineEdit()->setText(m_format);
    applyTextColor();
}

void KPatientDateEdit::SetPlaceholderText(const QString &t)
{
    lineEdit()->setPlaceholderText(t);
}

void KPatientDateEdit::SetTextMargins(int l, int t, int r, int b)
{
    lineEdit()->setTextMargins(l, t, r, b);
}

void KPatientDateEdit::applyTextColor()
{
    // Реф.: плейсхолдер → серый #999, значение → тёмный #3D3D3D.
    const bool placeholder = (lineEdit()->text() == specialValueText());
    setStyleSheet(placeholder ? QStringLiteral("color: rgb(153,153,153);")
                              : QStringLiteral("color: rgb(61,61,61);"));
}

void KPatientDateEdit::DateChangedSlot(const QDate &d)
{
    // Реф. @0x7b8230: игнор null, сброс pendingClear, перекрас.
    if (!d.isValid())
        return;
    m_pendingClear = false;
    applyTextColor();
}

void KPatientDateEdit::SlotToChangeTextColor(const QString &)
{
    applyTextColor();
}

void KPatientDateEdit::focusInEvent(QFocusEvent *e)
{
    // Реф. @0x7b8748: клик по пустому полю авто-сеет сегодняшнюю дату.
    if (!isReadOnly() && isEmpty()) {
        setDate(QDate::currentDate());
    }
    QDateEdit::focusInEvent(e);
}

void KPatientDateEdit::focusOutEvent(QFocusEvent *e)
{
    // Реф. @0x7b88c0: если помечено на очистку — назад в плейсхолдер.
    if (m_pendingClear) {
        m_pendingClear = false;
        clear();
    }
    QDateEdit::focusOutEvent(e);
}

void KPatientDateEdit::keyPressEvent(QKeyEvent *e)
{
    // Реф. @0x7b8990: цифра в пустое поле сеет сегодня; Backspace/Delete на полном
    // выделении → очистить + пометить pendingClear.
    const int k = e->key();
    if (k >= Qt::Key_0 && k <= Qt::Key_9 && isEmpty()) {
        setDate(QDate::currentDate());
        QDateEdit::keyPressEvent(e);
        return;
    }
    if ((k == Qt::Key_Backspace || k == Qt::Key_Delete)
        && lineEdit()->selectedText() == lineEdit()->text() && !lineEdit()->text().isEmpty()) {
        lineEdit()->setText(QString());
        m_pendingClear = true;
        return;
    }
    QDateEdit::keyPressEvent(e);
}
