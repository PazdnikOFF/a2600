#include "KTextEdit.h"

#include <QScrollBar>
#include <QMouseEvent>
#include <QStyle>
#include <QStyleOptionSlider>

KTextEdit::KTextEdit(QWidget *parent)
    : QTextEdit(parent)
{
    // Реф. ctor @0x815c60: mouse tracking + AsNeeded скроллбар. Фильтр ставит владелец —
    // в порте самоустанавливаем на viewport для самодостаточности.
    setAttribute(Qt::WA_MouseTracking, true);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    viewport()->installEventFilter(this);
    verticalScrollBar()->installEventFilter(this);
}

bool KTextEdit::IsOnVerticalScrollBar(const QPoint &pt) const
{
    // Реф. @0x815cf8.
    return verticalScrollBar()->geometry().contains(pt);
}

bool KTextEdit::eventFilter(QObject *obj, QEvent *e)
{
    // Реф. @0x815d20: тач-взаимодействие с вертикальным скроллбаром.
    QScrollBar *vbar = verticalScrollBar();
    if (e->type() == QEvent::MouseButtonPress) {
        QMouseEvent *me = static_cast<QMouseEvent *>(e);
        if (me->button() == Qt::LeftButton) {
            const QPoint pt = me->pos();
            if (obj == vbar || IsOnVerticalScrollBar(mapToParent(pt))) {
                // Позиция в координатах скроллбара.
                const QPoint bp = vbar->mapFromGlobal(me->globalPos());
                QStyleOptionSlider opt;
                opt.initFrom(vbar);
                opt.orientation = Qt::Vertical;
                opt.minimum = vbar->minimum();
                opt.maximum = vbar->maximum();
                opt.sliderPosition = vbar->value();
                opt.pageStep = vbar->pageStep();
                const QRect thumb = vbar->style()->subControlRect(
                    QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarSlider, vbar);
                if (bp.y() < thumb.top())
                    vbar->triggerAction(QAbstractSlider::SliderPageStepSub);
                else if (bp.y() > thumb.bottom())
                    vbar->triggerAction(QAbstractSlider::SliderPageStepAdd);
                else {
                    m_dragging = true;   // начало drag на ползунке
                    m_lastPos = bp;
                }
            }
        }
    } else if (e->type() == QEvent::MouseMove && m_dragging) {
        QMouseEvent *me = static_cast<QMouseEvent *>(e);
        const QPoint bp = vbar->mapFromGlobal(me->globalPos());
        const int dy = bp.y() - m_lastPos.y();
        if (vbar->height() > 0) {
            const int range = vbar->maximum() - vbar->minimum();
            vbar->setValue(vbar->value() + dy * range / vbar->height());
        }
        m_lastPos = bp;
    } else if (e->type() == QEvent::MouseButtonRelease && m_dragging) {
        m_dragging = false;   // конец drag
    }
    return QTextEdit::eventFilter(obj, e);
}
