#include "KImgPushButton.h"

#include <QPainter>

KImgPushButton::KImgPushButton(QWidget *parent)
    : QPushButton(parent)
{
    // Реф. ctor @0x5a8ca8: базовый QPushButton, 4 пустых QPixmap, m_loaded=0,
    // m_drawSize=(-1,-1). Ни mouse tracking, ни objectName, ни размер — всё в InitButtons.
}

int KImgPushButton::InitButtons(const QString &normal, const QString &hover,
                                const QString &checked, const QString &disable,
                                const QSize &size, bool useGivenSizeOnFail)
{
    // Реф. @0x5a8b78: фикс-размер + загрузка 4 пиксмапов.
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setMinimumSize(size);
    setMaximumSize(size);
    setFixedSize(size);

    const bool ok = m_normal.load(normal) & m_hover.load(hover)
                  & m_checked.load(checked) & m_disable.load(disable);
    if (ok) {
        m_loaded = true;
        // Реф.-баг: на успехе m_drawSize НЕ ставился (оставался -1,-1) → вырожденная
        // отрисовка. Ставим переданный size, чтобы пиксмап масштабировался корректно.
        m_drawSize = size;
        return 0;
    }
    // Реф. fail-путь: размер отрисовки по флагу.
    m_drawSize = useGivenSizeOnFail ? size : m_normal.size();
    update();
    return -1;
}

void KImgPushButton::paintEvent(QPaintEvent *e)
{
    // Реф. paintEvent @0x5a8e00: при незагруженных — сначала штатная кнопочная отрисовка.
    if (!m_loaded)
        QPushButton::paintEvent(e);

    // Приоритет: disabled > checked > hover > normal (реф. по флагам Qt).
    const QPixmap *pm;
    if (!isEnabled())
        pm = &m_disable;
    else if (isChecked())
        pm = &m_checked;
    else if (underMouse())
        pm = &m_hover;
    else
        pm = &m_normal;

    if (pm->isNull())
        return;
    QPainter p(this);
    // Реф.: drawPixmap(QRectF(0,0,drawSize), pm, source) — масштаб в целевой прямоугольник.
    p.drawPixmap(QRectF(0, 0, m_drawSize.width(), m_drawSize.height()), *pm, pm->rect());
}
