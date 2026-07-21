#include "KTempletTreeWidget.h"

#include <QHeaderView>

// ── KTempletTreeWidgetItem ───────────────────────────────────────────────────

KTempletTreeWidgetItem::KTempletTreeWidgetItem(const QString &title, const QString &id)
    : QTreeWidgetItem(QStringList{title})
    , m_id(id)
{
    // Реф. CreateTreeNode: flags 0x31 + начальный чек Unchecked.
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    setCheckState(0, Qt::Unchecked);
}

// ── KTempletTreeWidget ───────────────────────────────────────────────────────

KTempletTreeWidget::KTempletTreeWidget(QWidget *parent)
    : QTreeWidget(parent)
{
    // Реф. ctor @0x560a28: QTreeWidget + KObject (шина опущена) + scrollbar always-on.
    // InitLayout/InitConnection зовёт владелец — здесь вызовем сами для самодостаточности.
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    InitLayout();
    connect(this, &QTreeWidget::itemChanged, this, &KTempletTreeWidget::OnItemChanged);
}

void KTempletTreeWidget::InitLayout()
{
    // Реф. @0x55edb0.
    setExpandsOnDoubleClick(false);
    setColumnCount(4);
    setHeaderHidden(true);
    header()->setSectionResizeMode(0, QHeaderView::Stretch);
    header()->setSectionResizeMode(1, QHeaderView::Fixed); header()->resizeSection(1, 20);
    header()->setSectionResizeMode(2, QHeaderView::Fixed); header()->resizeSection(2, 30);
    header()->setSectionResizeMode(3, QHeaderView::Fixed); header()->resizeSection(3, 30);
}

void KTempletTreeWidget::updateChildren(QTreeWidgetItem *item, Qt::CheckState st)
{
    for (int i = 0; i < item->childCount(); ++i) {
        item->child(i)->setCheckState(0, st);
        updateChildren(item->child(i), st);
    }
}

void KTempletTreeWidget::updateParent(QTreeWidgetItem *item)
{
    QTreeWidgetItem *p = item->parent();
    if (!p)
        return;
    int checked = 0, total = p->childCount();
    for (int i = 0; i < total; ++i)
        if (p->child(i)->checkState(0) == Qt::Checked)
            ++checked;
    Qt::CheckState st = (checked == 0) ? Qt::Unchecked
                        : (checked == total) ? Qt::Checked : Qt::PartiallyChecked;
    p->setCheckState(0, st);
    updateParent(p);
}

void KTempletTreeWidget::OnItemChanged(QTreeWidgetItem *item, int col)
{
    // Реф. OnItemClicked/OnItemCheckd: col0 → три-состояние + emit при Checked.
    if (col != 0 || !item || m_guard)
        return;
    m_guard = true;   // антирекурсия (реф. DisConnection/InitConnection)
    const Qt::CheckState st = item->checkState(0);
    if (item->childCount() > 0)
        updateChildren(item, st);   // родитель → все дети
    updateParent(item);             // ребёнок → пересчёт родителя
    m_guard = false;

    if (st == Qt::Checked && item->childCount() == 0) {
        KTempletTreeWidgetItem *ti = static_cast<KTempletTreeWidgetItem *>(item);
        emit SigClickItem(ti->TemplateId());
    }
}

void KTempletTreeWidget::SetTreeProvider(std::function<void(KTempletTreeWidget *)> fn)
{
    m_treeProvider = std::move(fn);
    m_guard = true;
    clear();
    if (m_treeProvider)
        m_treeProvider(this);   // DEVICE-STUB: реф. KReportTemplateManager→KTemplateLibCfg
    expandAll();
    m_guard = false;
}
