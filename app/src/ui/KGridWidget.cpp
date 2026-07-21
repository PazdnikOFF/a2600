#include "KGridWidget.h"

#include <QGridLayout>

KGridWidget::KGridWidget(QWidget *parent)
    : QWidget(parent)
{
    // Реф. ctor @0x5a8350: пустой снапшот, cols=4, layout НЕ создаётся.
}

void KGridWidget::InitWidget(QGridLayout *layout, int columns)
{
    // Реф. @0x5a8888: фон-ребёнок + SetGridLayout + SetColumn + LoadAllWidgets.
    m_background = new QWidget(this);
    m_background->setObjectName(QStringLiteral("KGridWidgetBackground"));
    m_background->lower();
    SetGridLayout(layout);
    SetColumn(columns);
    LoadAllWidgets();
}

void KGridWidget::SetGridLayout(QGridLayout *layout)
{
    m_layout = layout;   // реф. @0x5a84a8 — НЕ владеет
}

void KGridWidget::SetColumn(int columns)
{
    m_columns = columns > 0 ? columns : 4;   // реф. @0x5a8778
}

void KGridWidget::LoadAllWidgets()
{
    // Реф. @0x5a8780: снять снапшот всех ячеек (widget, visible=true).
    m_cells.clear();
    if (!m_layout)
        return;
    for (int r = 0; r < m_layout->rowCount(); ++r) {
        for (int c = 0; c < m_layout->columnCount(); ++c) {
            QLayoutItem *it = m_layout->itemAtPosition(r, c);
            if (it && it->widget())
                m_cells.append(qMakePair(it->widget(), true));
        }
    }
}

void KGridWidget::MarkWidgetVisible(const QString &objectName, bool visible)
{
    // Реф. @0x5a83a0: только ставит флаг (переукладка — отдельным Relayout).
    for (auto &pr : m_cells)
        if (pr.first && pr.first->objectName() == objectName)
            pr.second = visible;
}

void KGridWidget::RemoveAllWidgets()
{
    // Реф. @0x5a8610.
    if (!m_layout)
        return;
    for (auto &pr : m_cells)
        if (pr.first)
            m_layout->removeWidget(pr.first);
}

void KGridWidget::Relayout()
{
    // Реф. @0x5a86b8: снять всё, затем плотно уложить только видимые (index по видимым).
    if (!m_layout)
        return;
    RemoveAllWidgets();
    int i = 0;
    for (auto &pr : m_cells) {
        if (!pr.first)
            continue;
        if (pr.second) {
            m_layout->addWidget(pr.first, i / m_columns, i % m_columns);
            pr.first->show();
            ++i;
        } else {
            pr.first->hide();
        }
    }
}

void KGridWidget::ShowAllRows()
{
    // Реф. @0x5a8570.
    for (auto &pr : m_cells)
        if (pr.first)
            pr.first->show();
}

void KGridWidget::HideSpecificRows(int fromRow)
{
    // Реф. @0x5a84b0: спрятать виджеты в строках >= fromRow.
    if (!m_layout)
        return;
    for (int r = fromRow; r < m_layout->rowCount(); ++r)
        for (int c = 0; c < m_layout->columnCount(); ++c) {
            QLayoutItem *it = m_layout->itemAtPosition(r, c);
            if (it && it->widget())
                it->widget()->hide();
        }
}

void KGridWidget::resizeEvent(QResizeEvent *e)
{
    // Реф. @0x5a89b8: фон-ребёнок держится под размер виджета.
    if (m_background)
        m_background->setFixedSize(size());
    QWidget::resizeEvent(e);
}
