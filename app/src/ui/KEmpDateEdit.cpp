#include "KEmpDateEdit.h"

#include <QLineEdit>
#include <QCalendarWidget>
#include <QFocusEvent>
#include <QMouseEvent>

KEmpDateEdit::KEmpDateEdit(QWidget *parent)
    : QDateEdit(parent)
{
    // Реф. ctor @0x715868: default-дата = сегодня; коннект dateChanged→DateChangedSlot.
    m_defaultDate = QDate::currentDate();
    connect(this, &QDateTimeEdit::dateChanged, this, &KEmpDateEdit::DateChangedSlot);
}

void KEmpDateEdit::showEmpty()
{
    // Реф. бланчит внутренний lineEdit; в порте — specialValueText на minimumDate (механизм
    // sibling-а KDateEdit, надёжно рендерится). Видимый результат тот же (пусто/placeholder).
    m_isEmpty = true;
    setSpecialValueText(m_placeholder.isEmpty() ? QStringLiteral(" ") : m_placeholder);
    QDateEdit::setDate(minimumDate());
}

void KEmpDateEdit::showValue()
{
    m_isEmpty = false;
    setSpecialValueText(QString());
}

int KEmpDateEdit::LineEditSectionAt(int pos) const
{
    // Реф. @0x716180: по формату — 3 раскладки.
    const QString fmt = displayFormat();
    if (fmt.startsWith(QLatin1String("MM")))       // MM/dd/yyyy
        return pos < 3 ? QDateTimeEdit::MonthSection : (pos < 6 ? QDateTimeEdit::DaySection : QDateTimeEdit::YearSection);
    if (fmt.startsWith(QLatin1String("dd")))       // dd/MM/yyyy
        return pos < 3 ? QDateTimeEdit::DaySection : (pos < 6 ? QDateTimeEdit::MonthSection : QDateTimeEdit::YearSection);
    // year-first (yyyy-MM-dd / yyyy/MM/dd)
    return pos < 5 ? QDateTimeEdit::YearSection : (pos < 8 ? QDateTimeEdit::MonthSection : QDateTimeEdit::DaySection);
}

void KEmpDateEdit::SetEmptyAble(bool able)
{
    // Реф. @0x715e60.
    m_canEmpty = able;
    if (able)
        showEmpty();
    update();
}

void KEmpDateEdit::clear()
{
    // Реф. @0x715fb0: форсировать пустое.
    m_canEmpty = true;
    showEmpty();
}

QDate KEmpDateEdit::date() const
{
    return (m_canEmpty && m_isEmpty) ? QDate() : QDateEdit::date();
}
QTime KEmpDateEdit::time() const
{
    return (m_canEmpty && m_isEmpty) ? QTime() : QDateEdit::time();
}
QDateTime KEmpDateEdit::dateTime() const
{
    return (m_canEmpty && m_isEmpty) ? QDateTime() : QDateEdit::dateTime();
}

void KEmpDateEdit::setDate(const QDate &d)
{
    // Реф. @0x715b78: валид-в-диапазоне → un-empty+база; sentinel+canEmpty → empty+бланк.
    if (d.isValid() && d != InvalidDate()) {
        showValue();
        QDateEdit::setDate(d);
    } else if (m_canEmpty) {
        showEmpty();
    } else {
        QDateEdit::setDate(d);
    }
}

void KEmpDateEdit::setTime(const QTime &t)
{
    if (t.isValid()) {
        showValue();
        QDateEdit::setTime(t);
    } else if (m_canEmpty) {
        showEmpty();
    } else {
        QDateEdit::setTime(t);
    }
}

void KEmpDateEdit::setDateTime(const QDateTime &dt)
{
    if (dt.isValid() && dt.date() != InvalidDate()) {
        showValue();
        QDateEdit::setDateTime(dt);
    } else if (m_canEmpty) {
        showEmpty();
    } else {
        QDateEdit::setDateTime(dt);
    }
}

void KEmpDateEdit::SetPlaceholderText(const QString &s)
{
    m_placeholder = s;
    if (lineEdit())
        lineEdit()->setPlaceholderText(s);
    if (m_isEmpty)
        setSpecialValueText(s.isEmpty() ? QStringLiteral(" ") : s);
}

void KEmpDateEdit::SetCursorPosition(int pos)
{
    if (lineEdit())
        lineEdit()->setCursorPosition(pos);
}

void KEmpDateEdit::paintEvent(QPaintEvent *e)
{
    // Реф. @0x716470: пустое → placeholder (в порте через specialValueText). Штатная отрисовка.
    QDateEdit::paintEvent(e);
}

void KEmpDateEdit::showEvent(QShowEvent *e)
{
    QDateEdit::showEvent(e);
}

void KEmpDateEdit::focusInEvent(QFocusEvent *e)
{
    // Реф. @0x716698: пустое+canEmpty+не-ro → заполнить дефолт-датой, подсветить кликнутую секцию.
    if (!isReadOnly() && m_canEmpty && m_isEmpty) {
        const int s = lineEdit() ? LineEditSectionAt(lineEdit()->cursorPosition()) : int(QDateTimeEdit::DaySection);
        showValue();
        QDateEdit::setDate(m_defaultDate);
        setCurrentSection(static_cast<QDateTimeEdit::Section>(s));
        m_guard = true;
        emit dateChanged(QDate());
    }
    QDateEdit::focusInEvent(e);
}

void KEmpDateEdit::focusOutEvent(QFocusEvent *e)
{
    // Реф. @0x716758: если фокус был, но правки не было (m_guard) → снова пусто.
    if (isReadOnly())
        return;
    if (m_guard) {
        m_guard = false;
        showEmpty();
    }
    QDateEdit::focusOutEvent(e);
}

void KEmpDateEdit::mousePressEvent(QMouseEvent *e)
{
    // Реф. @0x7165c8: базовый (клик-выбор секции); если было пусто+canEmpty+календарь открыт →
    // выставить текущую дату-время.
    const bool was = m_isEmpty;
    QDateEdit::mousePressEvent(e);
    if (was && m_canEmpty && calendarWidget() && calendarWidget()->isVisible())
        setDateTime(QDateTime::currentDateTime());
}

bool KEmpDateEdit::focusNextPrevChild(bool next)
{
    // Реф. @0x716650: пустое → Tab уходит из виджета; иначе Tab по секциям.
    if (m_canEmpty && m_isEmpty)
        return QWidget::focusNextPrevChild(next);
    return QDateEdit::focusNextPrevChild(next);
}

QValidator::State KEmpDateEdit::validate(QString &input, int &pos) const
{
    // Реф. @0x716678: пустое+canEmpty → Acceptable.
    if (m_isEmpty && m_canEmpty)
        return QValidator::Acceptable;
    return QDateEdit::validate(input, pos);
}

QSize KEmpDateEdit::sizeHint() const
{
    QSize s = QDateEdit::sizeHint();   // реф. @0x716308: +3 к ширине
    s.rwidth() += 3;
    return s;
}

QSize KEmpDateEdit::minimumSizeHint() const
{
    QSize s = QDateEdit::minimumSizeHint();
    s.rwidth() += 3;
    return s;
}

void KEmpDateEdit::DateChangedSlot(const QDate &d)
{
    // Реф. @0x7162c0: реальная правка (не null) → снять guard (значение коммитится).
    if (d != QDate())
        m_guard = false;
}
