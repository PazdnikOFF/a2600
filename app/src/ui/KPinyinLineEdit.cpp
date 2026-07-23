#include "KPinyinLineEdit.h"

#include <QBrush>
#include <QPainter>

KPinyinLineEdit::KPinyinLineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    // Реф. ctor @0x763d80: ровно QLineEdit(parent) + vptr. Никаких свойств не ставится —
    // стиль и раскладку задаёт владелец (KPinyinWidget::InitUI).
}

QString KPinyinLineEdit::text() const
{
    return QLineEdit::text();   // реф. @0x764220
}

void KPinyinLineEdit::insert(const QString &s)
{
    QLineEdit::insert(s);       // реф. @0x764248 — хвостовой вызов базы
}

void KPinyinLineEdit::setText(const QString &s)
{
    QLineEdit::setText(s);      // реф. @0x764250 — хвостовой вызов базы
}

void KPinyinLineEdit::focusOutEvent(QFocusEvent *e)
{
    // Реф. @0x764218: `b QWidget::focusOutEvent` — реализация QLineEdit СОЗНАТЕЛЬНО
    // пропускается, чтобы уход фокуса (на попап кандидатов) не сбрасывал выделение
    // и не завершал ввод. Вызываем через прародителя ровно так же.
    QWidget::focusOutEvent(e);
}

void KPinyinLineEdit::paintEvent(QPaintEvent *e)
{
    // Реф. @0x765ca0: заливка cursorRect() белым СПЛОШНЫМ, затем база.
    // Порядок именно такой (fillRect @0x765d14 → QLineEdit::paintEvent @0x765d20) —
    // воспроизводим дословно, хотя база и перерисовывает поверх.
    {
        QPainter p(this);
        p.fillRect(cursorRect(), QBrush(Qt::white, Qt::SolidPattern));
    }
    QLineEdit::paintEvent(e);
}
