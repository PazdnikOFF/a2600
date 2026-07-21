#include "KCheckBoxHeaderView.h"
#include "sys/KEnvConfig.h"

#include <QPainter>
#include <QMouseEvent>
#include <QPixmap>
#include <QDir>

KCheckBoxHeaderView::KCheckBoxHeaderView(int checkColumn, Qt::Orientation orientation, QWidget *parent)
    : QHeaderView(orientation, parent)
    , m_checkColumn(checkColumn)
{
    // Реф. ctor @0x7b2d00: init Unchecked, setSectionsClickable/StretchLastSection.
    setSectionsClickable(true);
}

QString KCheckBoxHeaderView::checkboxAsset(const QString &name) const
{
    return QDir(QString::fromStdString(KEnvConfig::GetInstance().GetReadOnlyBaseDir()))
        .absoluteFilePath(QStringLiteral("patient/checkbox/") + name);
}

void KCheckBoxHeaderView::paintSection(QPainter *p, const QRect &rect, int logicalIndex) const
{
    p->save();
    QHeaderView::paintSection(p, rect, logicalIndex);
    p->restore();
    if (logicalIndex != m_checkColumn)
        return;

    const bool hovering = m_hover && m_hoverSection == m_checkColumn;
    const bool checked = (m_checkState == Qt::Checked);
    QString name = checked ? QStringLiteral("checkbox_checked") : QStringLiteral("checkbox_unchecked");
    if (hovering)
        name += QStringLiteral("_hover");
    name += QStringLiteral(".png");
    const QPixmap pm(checkboxAsset(name));
    if (pm.isNull())
        return;
    const int x = rect.left() + (rect.width() - pm.width()) / 2;
    const int y = rect.top() + (rect.height() - pm.height()) / 2;
    p->drawPixmap(x, y, pm);
}

void KCheckBoxHeaderView::mousePressEvent(QMouseEvent *e)
{
    // Реф.: клик по секции 0 левой кнопкой → латч, съесть (подавить сортировку).
    if (logicalIndexAt(e->pos()) == m_checkColumn && (e->button() & Qt::LeftButton)) {
        m_pressedOnCheckbox = true;
        return;
    }
    QHeaderView::mousePressEvent(e);
}

void KCheckBoxHeaderView::mouseReleaseEvent(QMouseEvent *e)
{
    if (m_pressedOnCheckbox) {
        if (!m_partial)
            m_isChecked = !m_isChecked;   // тоггл
        else { m_isChecked = true; m_partial = false; }   // partial → Checked
        updateSection(m_checkColumn);
        m_checkState = m_isChecked ? Qt::Checked : Qt::Unchecked;
        emit SigCheckStausChange(m_isChecked ? 1 : 0);
        m_pressedOnCheckbox = false;
    } else {
        QHeaderView::mouseReleaseEvent(e);
        m_pressedOnCheckbox = false;
    }
}

void KCheckBoxHeaderView::mouseMoveEvent(QMouseEvent *e)
{
    m_hover = true;
    m_hoverSection = logicalIndexAt(e->pos());
    updateSection(m_checkColumn);
    QHeaderView::mouseMoveEvent(e);
}

void KCheckBoxHeaderView::leaveEvent(QEvent *e)
{
    m_hover = false;
    m_hoverSection = -1;
    updateSection(m_checkColumn);
    QHeaderView::leaveEvent(e);
}

void KCheckBoxHeaderView::SetCheckState(Qt::CheckState s)
{
    // Реф.: вью→заголовок (all→Checked / none→Unchecked / mixed→PartiallyChecked).
    m_isChecked = (s != Qt::Unchecked);
    m_partial = (s == Qt::PartiallyChecked);
    m_checkState = s;
    updateSection(m_checkColumn);
}

void KCheckBoxHeaderView::UpdateCheckboxHeader()
{
    m_hover = false;
    m_hoverSection = -1;
    updateSection(m_checkColumn);
}
