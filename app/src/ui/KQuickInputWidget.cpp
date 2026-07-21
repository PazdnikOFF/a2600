#include "KQuickInputWidget.h"

#include <QListView>
#include <QStringListModel>
#include <QVBoxLayout>
#include <QKeyEvent>

KQuickInputWidget::KQuickInputWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("KQuickInputWidget"));
    m_model = new QStringListModel(this);
    m_list = new QListView(this);
    m_list->setObjectName(QStringLiteral("quickInputList"));
    m_list->setModel(m_model);
    m_list->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QVBoxLayout *v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->addWidget(m_list);

    connect(m_list, &QListView::clicked, this, &KQuickInputWidget::onClicked);
}

void KQuickInputWidget::SetItems(const QStringList &items)
{
    m_model->setStringList(items);
    if (!items.isEmpty())
        m_list->setCurrentIndex(m_model->index(0, 0));
    resize(width() > 0 ? width() : 200, GetListViewHeight());
}

int KQuickInputWidget::Count() const
{
    return m_model->rowCount();
}

int KQuickInputWidget::GetListViewHeight() const
{
    // Высота под число строк (реф. GetListViewHeight), с потолком.
    const int rows = m_model->rowCount();
    const int rowH = m_list->sizeHintForRow(0) > 0 ? m_list->sizeHintForRow(0) : 24;
    const int shown = qMin(rows, 8);   // потолок видимых строк
    return qMax(rowH, shown * rowH + 4);
}

int KQuickInputWidget::CurrentRow() const
{
    return m_list->currentIndex().row();
}

void KQuickInputWidget::MoveCursor(int delta)
{
    const int n = m_model->rowCount();
    if (n == 0)
        return;
    int r = CurrentRow() + delta;
    if (r < 0) r = 0;
    if (r >= n) r = n - 1;
    m_list->setCurrentIndex(m_model->index(r, 0));
}

void KQuickInputWidget::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_Up:    MoveCursor(-1); return;
    case Qt::Key_Down:  MoveCursor(+1); return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (CurrentRow() >= 0) { emit itemSelect(CurrentRow()); hide(); }
        return;
    case Qt::Key_Escape: hide(); return;
    default: QWidget::keyPressEvent(e);
    }
}

void KQuickInputWidget::onClicked(const QModelIndex &idx)
{
    if (idx.isValid()) {
        emit itemSelect(idx.row());
        hide();
    }
}
