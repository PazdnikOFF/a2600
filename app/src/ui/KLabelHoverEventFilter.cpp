#include "KLabelHoverEventFilter.h"

#include <QEvent>
#include <QToolTip>
#include <QWidget>

KLabelHoverEventFilter::KLabelHoverEventFilter(QWidget *w, const QString &text, int offset)
    : QObject(w)
    , m_widget(w)
    , m_text(text)
    , m_offset(offset)
{
}

QPoint KLabelHoverEventFilter::TipPoint() const
{
    if (!m_widget)
        return QPoint();
    // Реф.: h = crect.y2 - crect.y1 + 1 (то есть height()), сдвиг влево 10, вверх 2*h - offset.
    const QPoint gp = m_widget->mapToGlobal(QPoint(0, 0));
    return QPoint(gp.x() - 10, gp.y() - 2 * m_widget->height() + m_offset);
}

bool KLabelHoverEventFilter::eventFilter(QObject *obj, QEvent *e)
{
    const QEvent::Type t = e->type();
    if (t == QEvent::ToolTip)
        QToolTip::showText(TipPoint(), m_text, m_widget);   // реф.: БЕЗ return — идём дальше

    // Реф. гейт: `cmp w,#0x27; b.gt` — Enter/Leave живут только при offset <= 39.
    if (m_offset > 39)
        return QObject::eventFilter(obj, e);

    if (t == QEvent::Enter) {
        QToolTip::showText(TipPoint(), m_text, m_widget);
        return true;
    }
    if (t == QEvent::Leave) {
        QToolTip::showText(TipPoint(), QString(), nullptr);   // гашение
        return true;
    }
    return QObject::eventFilter(obj, e);
}
