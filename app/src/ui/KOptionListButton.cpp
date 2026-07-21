#include "KOptionListButton.h"

#include <QPainter>
#include <QMouseEvent>
#include <QLinearGradient>
#include <QPainterPath>
#include <QRegion>

KOptionListButton::KOptionListButton(QWidget *parent)
    : KOptionListButton(QStringList(), parent)   // реф. @0x3d9600 → делегирует
{
}

KOptionListButton::KOptionListButton(const QStringList &options, QWidget *parent)
    : QWidget(parent)
{
    // Реф. ctor @0x3d9380: дефолты (index=-1, border=1, radius=9, цвета, шрифт), затем SetOptions.
    setAttribute(Qt::WA_MouseTracking, true);
    SetOptions(options);
}

void KOptionListButton::SetOptions(const QStringList &options)
{
    // Реф. @0x3d92c8.
    m_options = options;
    m_displayTexts = options;
    SetIndex(options.isEmpty() ? -1 : 0);   // авто-выбор первой
    update();
}

QString KOptionListButton::Option(int index) const
{
    if (index < 0 || index >= m_options.size())
        return QString();
    return m_options[index];
}

void KOptionListButton::SetIndex(int index)
{
    // Реф. @0x3d8950: коммит только при изменении.
    if (index == m_index)
        return;
    m_index = index;
    emit IndexChanged(index);
    update();
}

void KOptionListButton::SetOptionData(int index, const QVariant &data)
{
    // Реф. @0x3d9678: лениво растим список данных до числа опций.
    if (index < 0)
        return;
    while (m_optionData.size() < m_options.size())
        m_optionData.append(QVariant());
    if (index < m_optionData.size())
        m_optionData[index] = data;
}

QVariant KOptionListButton::GetOptionData(int index) const
{
    if (index < 0 || index >= m_optionData.size())
        return QVariant();
    return m_optionData[index];
}

int KOptionListButton::GetTheFirstOptionIndexWithData(const QVariant &data) const
{
    for (int i = 0; i < m_optionData.size(); ++i)
        if (m_optionData[i] == data)
            return i;
    return -1;
}

int KOptionListButton::IndexAtPos(int x, int y) const
{
    // Реф. @0x3d8a30: cellW = width/count; clamp; -1 если вне.
    Q_UNUSED(y);
    const int n = m_options.size();
    if (n <= 0)
        return -1;
    if (x < 0 || x >= width())
        return -1;
    const int cellW = width() / n;
    if (cellW <= 0)
        return -1;
    int idx = x / cellW;
    if (idx < 0) idx = 0;
    if (idx > n - 1) idx = n - 1;
    return idx;
}

void KOptionListButton::mouseReleaseEvent(QMouseEvent *e)
{
    // Реф. @0x3d8aa8: клик по ячейке → выбор.
    const int idx = IndexAtPos(e->pos().x(), e->pos().y());
    if (idx != -1)
        SetIndex(idx);
    QWidget::mouseReleaseEvent(e);
}

void KOptionListButton::mouseMoveEvent(QMouseEvent *e)
{
    // Реф. @0x3d9af8: тултип с полным текстом, если ячейка была урезана эллипсисом.
    const int idx = IndexAtPos(e->pos().x(), e->pos().y());
    if (idx >= 0 && idx < m_options.size() && idx < m_displayTexts.size()
        && m_displayTexts[idx] != m_options[idx])
        setToolTip(m_options[idx]);
    else
        setToolTip(QString());
    QWidget::mouseMoveEvent(e);
}

void KOptionListButton::ResizeGradient()
{
    // Реф. @0x3d8b88: две вертикальные QLinearGradient (0→height).
    QLinearGradient gn(0, 0, 0, height());
    gn.setColorAt(0, m_normalColor0);
    gn.setColorAt(1, m_normalColor1);
    m_normalBrush = QBrush(gn);

    QLinearGradient gh(0, 0, 0, height());
    gh.setColorAt(0, m_highlightColor0);
    gh.setColorAt(1, m_highlightColor1);
    m_highlightBrush = QBrush(gh);
}

void KOptionListButton::ResizeMask()
{
    // Реф. @0x3d8d00: скруглённый путь (radius-3) → QRegion → setMask.
    QPainterPath path;
    const int r = qMax(0, m_radius - 3);
    path.addRoundedRect(QRectF(0, 0, width(), height()), r, r);
    setMask(QRegion(path.toFillPolygon().toPolygon()));
}

void KOptionListButton::showEvent(QShowEvent *e)
{
    ResizeMask();
    ResizeGradient();
    QWidget::showEvent(e);
}

void KOptionListButton::resizeEvent(QResizeEvent *e)
{
    ResizeMask();
    ResizeGradient();
    QWidget::resizeEvent(e);
}

void KOptionListButton::DrawBackground(QPainter &p)
{
    // Реф. @0x3d8f20: по ячейке — заливка; выбранная — highlight-кисть.
    const int n = m_options.size();
    if (n <= 0)
        return;
    const int cellW = width() / n;
    for (int i = 0; i < n; ++i) {
        const int x = i * cellW;
        const int w = (i == n - 1) ? (width() - x) : cellW;
        p.fillRect(QRect(x, 0, w, height()), i == m_index ? m_highlightBrush : m_normalBrush);
    }
}

void KOptionListButton::DrawText(QPainter &p)
{
    // Реф. @0x3d9850: эллипсис по ширине ячейки, центр; выбранная — highlight-цвет.
    const int n = m_options.size();
    if (n <= 0)
        return;
    p.setFont(m_font);
    const int cellW = width() / n;
    for (int i = 0; i < n; ++i) {
        const int x = i * cellW;
        const int w = (i == n - 1) ? (width() - x) : cellW;
        const QString elided = m_fontMetrics.elidedText(m_options[i], Qt::ElideRight, w - 4);
        if (i < m_displayTexts.size())
            m_displayTexts[i] = elided;   // кэш для тултипа
        p.setPen(i == m_index ? m_textHighlightColor : m_textColor);
        p.drawText(QRect(x, 0, w, height()), Qt::AlignCenter, elided);
    }
}

void KOptionListButton::DrawGrid(QPainter &p)
{
    // Реф. @0x3d9080: перо normalColor0, прозрачная заливка; скруглённая рамка + вертикали.
    const int n = m_options.size();
    p.setPen(m_normalColor0);
    p.setBrush(Qt::NoBrush);
    const qreal inset = m_borderWidth;
    p.drawRoundedRect(QRectF(inset / 2.0, inset / 2.0,
                             width() - inset, height() - inset),
                      m_radius, m_radius);
    if (n > 1) {
        const int cellW = width() / n;
        for (int i = 1; i < n; ++i) {
            const int x = i * cellW;
            p.drawLine(x, 0, x, height());
        }
    }
}

void KOptionListButton::paintEvent(QPaintEvent *)
{
    // Реф. @0x3d9a78: AA → DrawBackground → DrawText → DrawGrid.
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    DrawBackground(p);
    DrawText(p);
    DrawGrid(p);
}
