#include "KOsdSubMenu.h"
#include "KFrame.h"

#include <QVBoxLayout>
#include <QListWidget>

KOsdSubMenu::KOsdSubMenu(QWidget *parent, bool bAddReturnBtn)
    : KOsdMenuBase(parent)
    , m_bAddReturnBtn(bAddReturnBtn)
{
    // Реф. ctor @0x47b4f0: свой шелл (270 wide, translucent) + InitConnect.
    setObjectName(QStringLiteral("KOsdSubMenu"));
    setMinimumWidth(270);
    setMaximumWidth(270);
    setStyleSheet(QStringLiteral("color: rgb(160, 160, 160);"));
    setAttribute(Qt::WA_TranslucentBackground, true);

    QWidget *content = ContentArea();
    m_layout = new QVBoxLayout(content);
    m_layout->setObjectName(QStringLiteral("verticalLayout"));
    m_layout->setSpacing(0);
    m_layout->setContentsMargins(0, 0, 0, 0);

    m_listWidget = new QListWidget(content);
    m_listWidget->setObjectName(QStringLiteral("listWidget"));
    m_listWidget->setFocusPolicy(Qt::NoFocus);
    m_listWidget->setStyleSheet(QStringLiteral("QListWidget::item{margin-left:4px; margin-right:4px;}"));
    m_listWidget->setAttribute(Qt::WA_TranslucentBackground, true);
    m_layout->addWidget(m_listWidget);

    connect(m_listWidget, &QListWidget::clicked, this, &KOsdSubMenu::ItemClicked);
}

void KOsdSubMenu::AddItem(QWidget *w)
{
    // Реф. @0x47b9c8: setParent + insertItem + setItemWidget + sizeHint.
    if (!w)
        return;
    w->setParent(m_listWidget);
    QListWidgetItem *item = new QListWidgetItem();
    item->setSizeHint(w->sizeHint());
    m_listWidget->insertItem(m_listWidget->count(), item);
    m_listWidget->setItemWidget(item, w);
    m_items.append(w);
    if (KFrame *f = qobject_cast<KFrame *>(w))
        f->SetLocatedMenu(this);   // реф.: строка знает своё меню-владельца (для EnterMenu)
}

void KOsdSubMenu::AddItem(const KOsdSpinConfig &cfg)
{
    // Реф. @0x47bd20: строит спин-виджет (KOsdSpin02) → host. В порте — KOsdSpin.
    AddItem(new KOsdSpin(cfg, m_listWidget));
}

void KOsdSubMenu::AddItem(const KOsdDoubleSpinConfig &cfg)
{
    AddItem(new KOsdDoubleSpin(cfg, m_listWidget));
}

void KOsdSubMenu::AddItem(const KOsdLabelConfig &cfg)
{
    // Реф. @0x47baa0: строит KOsdLabel из конфига → базовый хост.
    AddItem(new KOsdLabel(cfg, m_listWidget));
}

void KOsdSubMenu::InitWidget(const QPoint &pos)
{
    // Реф. @0x47bfe0: width 270, height=count*44, клип к экрану 1080, move.
    m_pos = pos;
    const int height = qMax(1, m_items.size()) * 44;
    resize(270, height);
    int y = pos.y();
    while (y + height > 1080 && y > 0)
        y -= 32;
    move(pos.x(), y);
    // Реф.: строки-метки получают стартовую точку меню для позиционирования под-подменю.
    for (QWidget *w : m_items)
        if (KFrame *f = qobject_cast<KFrame *>(w))
            f->SetStartPoint(pos);
}

void KOsdSubMenu::RefreshSelectedItem(int oldIdx)
{
    if (oldIdx >= 0 && oldIdx < m_items.size()) {
        if (KOsdSpin *s = qobject_cast<KOsdSpin *>(m_items[oldIdx])) Q_UNUSED(s);   // спин сам без Select
    }
    m_listWidget->setCurrentRow(m_selectedIndex);
    update();
}

void KOsdSubMenu::UpKeyAct()
{
    if (m_items.isEmpty())
        return;
    const int old = m_selectedIndex;
    m_selectedIndex = (m_selectedIndex - 1 < 0) ? m_items.size() - 1 : m_selectedIndex - 1;
    RefreshSelectedItem(old);
}

void KOsdSubMenu::DownKeyAct()
{
    if (m_items.isEmpty())
        return;
    const int old = m_selectedIndex;
    m_selectedIndex = (m_selectedIndex + 1 >= m_items.size()) ? 0 : m_selectedIndex + 1;
    RefreshSelectedItem(old);
}

void KOsdSubMenu::ItemClicked(const QModelIndex &idx)
{
    if (!idx.isValid())
        return;
    const int old = m_selectedIndex;
    m_selectedIndex = idx.row();
    RefreshSelectedItem(old);
}

void KOsdSubMenu::ConfirmKeyAct()
{
    // Реф. @0x47b080: активация текущего элемента. Для single-select-меток (KFrame-подкласс):
    // Selected → ConfirmKeyAct(seam) → если IsSingleSelectLabel, коммит checked-индекса + refresh.
    // Спины (KOsdSpin/KOsdDoubleSpin — НЕ KFrame) сюда не попадают (cast промахивается).
    const int row = m_selectedIndex;
    if (row < 0 || row >= m_items.size())
        return;
    if (KFrame *f = qobject_cast<KFrame *>(m_items[row])) {
        f->Selected();                 // реф. item.Selected() (vt+0x1a0)
        f->ConfirmKeyAct(row);         // реф. item.ConfirmKeyAct(cursor) (vt+0x1e8) → DEVICE-seam
        if (f->IsSingleSelectLabel()) {   // реф. vt+0x1f0
            m_checkedIndex = m_selectedIndex;
            RefreshCheckedItem();
        }
    }
}

void KOsdSubMenu::InitCheckedItem(int idx)
{
    // Реф. @0x47b078: курсор+чек на idx, затем refresh иконок.
    m_selectedIndex = idx;
    m_checkedIndex = idx;
    RefreshCheckedItem();
}

void KOsdSubMenu::RefreshCheckedItem()
{
    // Реф. @0x47af90: чекнутая строка → Checked+Selected; прочие → UnSelected+UnChecked.
    for (int i = 0; i < m_items.size(); ++i) {
        KFrame *f = qobject_cast<KFrame *>(m_items[i]);
        if (!f)
            continue;
        if (i == m_checkedIndex) {
            f->Checked();
            f->Selected();
        } else {
            f->UnSelected();
            f->UnChecked();
        }
    }
    if (m_checkedIndex >= 0 && m_checkedIndex < m_items.size())
        m_listWidget->setCurrentRow(m_checkedIndex);
    update();
}

void KOsdSubMenu::SetValue(int row, int value)
{
    // Реф. SetValue: фан-аут в hosted-спин (device→UI). Тихий сеттер.
    if (row < 0 || row >= m_items.size())
        return;
    if (KOsdSpin *s = qobject_cast<KOsdSpin *>(m_items[row]))
        s->SetIntValue(value);
}

void KOsdSubMenu::SetValue(int row, double value)
{
    if (row < 0 || row >= m_items.size())
        return;
    if (KOsdDoubleSpin *s = qobject_cast<KOsdDoubleSpin *>(m_items[row]))
        s->SetDoubleValue(value);
}
