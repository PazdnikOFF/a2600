#include "KPagePushButton.h"

#include <QPainter>
#include <QMouseEvent>

KPagePushButton::KPagePushButton(QWidget *parent)
    : QPushButton(parent)
{
    // Реф. ctor @0x7b72e8: базовый QPushButton + WA_MouseTracking, чтобы mouseMoveEvent
    // отрабатывал hover без зажатой кнопки. Картинки/размер — в InitButton.
    setMouseTracking(true);
}

void KPagePushButton::InitButton(const QMap<QString, QString> &icons)
{
    // Реф. @0x7b73e0: достаёт из карты 3 пути, грузит 3 пиксмапа, размер под normal.
    m_normalPath = icons.value(QStringLiteral("normalIcon"));
    m_hoverPath = icons.value(QStringLiteral("hoverIcon"));
    m_disablePath = icons.value(QStringLiteral("disableIcon"));

    m_normal.load(m_normalPath);
    m_hover.load(m_hoverPath);
    m_disable.load(m_disablePath);

    if (!m_normal.isNull())
        setFixedSize(m_normal.size());   // реф. setFixedSize(normalPixmap.size())

    m_pressed = false;   // реф. обнуление слова состояния +0xa8
}

void KPagePushButton::paintEvent(QPaintEvent *)
{
    // Реф. @0x7b7068: рисуем один из 3 пиксмапов по состоянию (без stylesheet/QIcon).
    const QPixmap *pm = &m_normal;
    if (!isEnabled())
        pm = &m_disable;
    else if (m_hoverState)
        pm = &m_hover;

    if (pm->isNull())
        return;
    QPainter p(this);
    p.drawPixmap(rect(), *pm, pm->rect());
}

void KPagePushButton::mousePressEvent(QMouseEvent *e)
{
    // Реф. @0x7b7be8: setFocus + pressed=hover=1, затем базовый (эмитит clicked по release).
    setFocus();
    m_pressed = true;
    m_hoverState = true;
    update();
    QPushButton::mousePressEvent(e);
}

void KPagePushButton::mouseMoveEvent(QMouseEvent *e)
{
    // Реф. @0x7b71a0: ручной hit-test. Внутри → pressed+hover; снаружи → hover=hasFocus.
    if (rect().contains(e->pos())) {
        m_pressed = true;
        m_hoverState = true;
    } else {
        m_pressed = false;
        m_hoverState = hasFocus();
    }
    update();
    QPushButton::mouseMoveEvent(e);
}

void KPagePushButton::focusInEvent(QFocusEvent *e)
{
    m_hoverState = true;   // реф. +0xa9=1
    update();
    QPushButton::focusInEvent(e);
}

void KPagePushButton::focusOutEvent(QFocusEvent *e)
{
    // Реф.: если не нажата и нет фокуса — снять hover.
    if (!m_pressed && !hasFocus())
        m_hoverState = false;
    update();
    QPushButton::focusOutEvent(e);
}

void KPagePushButton::RefreshBtnOfLeave()
{
    // Реф. @0x7b72b0: снять hover если нет фокуса, всегда снять pressed, перерисовать.
    if (!hasFocus())
        m_hoverState = false;
    m_pressed = false;
    update();
}
