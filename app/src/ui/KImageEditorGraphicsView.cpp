#include "KImageEditorGraphicsView.h"

#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>

#include <algorithm>   // std::max
#include <cmath>       // std::hypot (libstdc++ не тянет транзитивно)

namespace {
// Реф. пер-тип hotspot-офсет (чтобы «остриё» попадало на клик): {1:(13,13),2:(0,13),
// 3:(0,0),4:(13,0),5:(0,0)}.
QPoint hotspot(int type)
{
    switch (type) {
    case KImageEditorGraphicsView::ArrowRightDown: return QPoint(13, 13);
    case KImageEditorGraphicsView::ArrowRightUp:   return QPoint(0, 13);
    case KImageEditorGraphicsView::ArrowLeftUp:    return QPoint(0, 0);
    case KImageEditorGraphicsView::ArrowLeftDown:  return QPoint(13, 0);
    case KImageEditorGraphicsView::Point:          return QPoint(0, 0);
    default: return QPoint(0, 0);
    }
}
} // namespace

KImageEditorGraphicsView::KImageEditorGraphicsView(QWidget *parent)
    : QGraphicsView(parent)
{
    // Реф. ctor: скроллбары off, NoDrag.
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setDragMode(QGraphicsView::NoDrag);
    setBackgroundBrush(QBrush(QColor(1, 1, 1)));   // near-black (как placeholder)
    InitSceneAndBackground();
}

void KImageEditorGraphicsView::InitSceneAndBackground()
{
    // Реф. InitSceneAndBackground: сцена + фон-item z=-1.
    m_scene = new QGraphicsScene(this);
    m_bgItem = new QGraphicsPixmapItem();
    m_bgItem->setZValue(-1);
    m_scene->addItem(m_bgItem);
    setScene(m_scene);
}

QPixmap KImageEditorGraphicsView::MakeCursorPixmap(int type)
{
    // Глиф-замена device-иконок стрелок/точки (реф. GetCursorImgPath из GetReadOnlyBaseDir).
    QPixmap pm(26, 26);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(QColor(0, 205, 209), 2.5);   // cyan (акцент темы)
    p.setPen(pen);
    p.setBrush(QColor(0, 205, 209));
    auto arrow = [&](QPoint tail, QPoint tip) {
        p.drawLine(tail, tip);
        // головка: два коротких штриха у tip
        QPointF d = QPointF(tip - tail);
        double len = std::max(1.0, std::hypot(d.x(), d.y()));
        QPointF u(d.x() / len, d.y() / len);
        QPointF n(-u.y(), u.x());
        QPointF b = QPointF(tip) - u * 8.0;
        p.drawLine(tip, (b + n * 5.0).toPoint());
        p.drawLine(tip, (b - n * 5.0).toPoint());
    };
    switch (type) {
    case ArrowRightDown: arrow(QPoint(3, 3), QPoint(23, 23)); break;
    case ArrowRightUp:   arrow(QPoint(3, 23), QPoint(23, 3)); break;
    case ArrowLeftUp:    arrow(QPoint(23, 23), QPoint(3, 3)); break;
    case ArrowLeftDown:  arrow(QPoint(23, 3), QPoint(3, 23)); break;
    case Point:          p.drawEllipse(QPoint(13, 13), 5, 5); break;
    default: break;
    }
    return pm;
}

void KImageEditorGraphicsView::SetBackgroundPixmap(const QPixmap &pm, int w, int h)
{
    if (!m_bgItem)
        return;
    m_bgItem->setPixmap(pm);
    m_bgItem->setPos(0, 0);
    m_scene->setSceneRect(0, 0, pm.width(), pm.height());
    // Реф. m_drawableRect = sceneRect с отступом ARROW_DRAW_MARGIN (берём небольшой инсет).
    const qreal margin = 8;
    m_drawableRect = QRectF(margin, margin,
                            qMax<qreal>(0, pm.width() - 2 * margin),
                            qMax<qreal>(0, pm.height() - 2 * margin));
    m_bgSize = QSize(w > 0 ? w : pm.width(), h > 0 ? h : pm.height());
}

void KImageEditorGraphicsView::SetBackgroundPixmap(const QString &path, int w, int h)
{
    SetBackgroundPixmap(QPixmap(path), w, h);
}

void KImageEditorGraphicsView::SetCursorType(int type)
{
    // Реф. SetCursorType: +0xa0 + QCursor виджета = пиксмап инструмента (Lock → стрелка ОС).
    m_cursorType = type;
    if (type == Lock)
        setCursor(Qt::ArrowCursor);
    else
        setCursor(QCursor(MakeCursorPixmap(type)));
}

void KImageEditorGraphicsView::AddCursorPixmapItem(int x, int y, int type)
{
    // Реф. AddCursorPixmapItem: item + setPos(scene - hotspot) + push в историю.
    if (!m_scene)
        return;
    QGraphicsPixmapItem *it = new QGraphicsPixmapItem(MakeCursorPixmap(type));
    const QPoint hs = hotspot(type);
    it->setPos(x - hs.x(), y - hs.y());
    m_scene->addItem(it);
    CursorItem ci;
    ci.item = it; ci.centerX = x; ci.centerY = y; ci.type = type;
    m_history.append(ci);
}

void KImageEditorGraphicsView::UndoLastItem()
{
    // Реф. UndoLastItem (в бинарнике FIFO-квирк; в порте — последняя, UI-намерение).
    if (m_history.isEmpty())
        return;
    CursorItem ci = m_history.takeLast();
    if (m_scene && ci.item)
        m_scene->removeItem(ci.item);
    delete ci.item;
}

void KImageEditorGraphicsView::RemoveAllCursorPixmapItem()
{
    for (const CursorItem &ci : m_history) {
        if (m_scene && ci.item)
            m_scene->removeItem(ci.item);
        delete ci.item;
    }
    m_history.clear();
}

void KImageEditorGraphicsView::mousePressEvent(QMouseEvent *e)
{
    // Реф. mousePress: клик в drawableRect при type!=Lock → расставить метку + сигнал.
    const QPointF pt = mapToScene(e->pos());
    if (m_cursorType != Lock && m_drawableRect.contains(pt)) {
        AddCursorPixmapItem(int(pt.x()), int(pt.y()), m_cursorType);
        emit DrawEventOccurs();
    }
    QGraphicsView::mousePressEvent(e);
}
