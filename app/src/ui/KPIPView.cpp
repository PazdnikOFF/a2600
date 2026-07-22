#include "KPIPView.h"

#include <QPainter>

KPIPViewRect::KPIPViewRect(QWidget *parent)
    : QLabel(parent)
{
    // Реф. ctor @0x70b1c8: autoFillBackground + opacity 1.0 + translucent-фон.
    setAutoFillBackground(true);
    setWindowOpacity(1.0);
    setAttribute(Qt::WA_TranslucentBackground, true);
}

void KPIPViewRect::refresh()
{
    // Реф.: KDisplayOption::SetFramePosition(this, m_rect) = move+resize; затем repaint.
    move(m_rect.topLeft());
    resize(m_rect.size());
    repaint();
}

void KPIPViewRect::setVisible(bool v)
{
    // Реф.: если показываем — сперва refresh (переставить/пересчитать), затем база.
    if (v)
        refresh();
    QLabel::setVisible(v);
}

void KPIPViewRect::paintEvent(QPaintEvent *)
{
    // Реф. paintEvent: CompositionMode_Clear «пробивает» (скруглённую) прозрачную дыру во весь
    // виджет. m_radiusRect==null → острые углы (fillRect); иначе drawRoundRect(.x(),.y()).
    QPainter p(this);
    p.setCompositionMode(QPainter::CompositionMode_Clear);
    const QRectF r(0, 0, width(), height());
    if (m_radiusRect == QRect(0, 0, -1, -1)) {
        p.fillRect(r, QBrush(Qt::SolidPattern));
    } else {
        p.setBrush(QBrush(Qt::SolidPattern));
        p.drawRoundRect(r, m_radiusRect.x(), m_radiusRect.y());   // реф. legacy % rounding
    }
}

KPIPView::KPIPView(QWidget *parent)
    : QObject(parent)
{
    // Реф. ctor: создаёт один KPIPViewRect, parented к widget-аргументу.
    m_rect = new KPIPViewRect(parent);
}

void KPIPView::refresh(const QRect &r)
{
    // Реф.: если r валиден (!= null) — сохранить в m_geom; затем rect->refresh.
    if (r != QRect(0, 0, -1, -1)) {
        m_geom = r;
        if (m_rect)
            m_rect->setRect(r);
    }
    if (m_rect)
        m_rect->refresh();
}

void KPIPView::setRadius(const QRect &r)
{
    if (m_rect)
        m_rect->setRadius(r);
}

void KPIPView::setVisible(bool v)
{
    if (v)
        refresh();
    if (m_rect)
        m_rect->setVisible(v);
}
