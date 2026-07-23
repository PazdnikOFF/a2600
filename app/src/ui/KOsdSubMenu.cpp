#include "KOsdSubMenu.h"
#include "KFrame.h"

#include <QVBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QKeyEvent>

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
    // Реф. @0x47bd20: `new(nothrow) KOsdSpin02(cfg, this)` — именно KOsdSpin02, а НЕ KOsdSpin
    // (последний в прошивке нигде не создаётся, см. комментарий в KOsdSpin02.h).
    AddItem(new KOsdSpin02(cfg, m_listWidget));
}

void KOsdSubMenu::AddAReturnBtnInTheEndIfNeeded()
{
    // Реф. @0x47bf60: гейт по флагу ctor-а + защёлка «уже добавлена».
    if (!m_bAddReturnBtn || m_returnBtnAdded)
        return;
    AddItem(new KOsdReturnLabel());
    m_returnBtnAdded = true;
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

void KOsdSubMenu::AddItem(const KOsdStatusLabelConfig &cfg)
{
    // Реф.: строит KOsdStatusLabel из конфига → базовый хост.
    AddItem(new KOsdStatusLabel(cfg, m_listWidget));
}

void KOsdSubMenu::AddItem(const KOsdSingleSelectLabelConfig &cfg)
{
    // Реф.: single-select-метка в НЕ-single-select-меню (KbuttonSetting). action-колбэк —
    // DEVICE-seam (SendToMainCtrl), в порте no-op (nullptr).
    AddItem(new KOsdSingleSelectLabel(cfg, m_listWidget, nullptr));
}

void KOsdSubMenu::InitWidget(const QPoint &pos)
{
    // Реф. @0x47bfe0: ПЕРВЫМ делом дописывается строка «назад» (если её просили), и только
    // потом считается высота — иначе она не попала бы в размер меню.
    AddAReturnBtnInTheEndIfNeeded();
    m_pos = pos;
    const int height = qMax(1, m_listWidget->count()) * 44;   // реф. count()*44 (0x2c), width 270 (0x10e)
    resize(270, height);
    // Реф.: пока низ меню за экраном (0x438 = 1080), поднимаем на 32. Ограничения «не
    // выше нуля» в реф. НЕТ — y может уйти отрицательным (цикл всё равно конечен).
    int y = pos.y();
    while (y + height > 1080)
        y -= 32;
    move(pos.x(), y);
    // Реф. InitItemPosition @0x47acc8: строки получают стартовую точку меню (для под-подменю).
    for (QWidget *w : m_items)
        if (KFrame *f = qobject_cast<KFrame *>(w))
            f->SetStartPoint(pos);
    // Реф. хвост InitWidget @0x47c088: сразу же подсветить строки по текущему курсору —
    // именно здесь строка «назад» получает свой пиксмап вместо текста-плейсхолдера.
    RefreshSelectedItem();
}

KFrame *KOsdSubMenu::GetSelectedMenuItem() const
{
    // Реф. @0x47ac88 → GetMenuItem(m_selectedIndex).
    if (m_selectedIndex < 0 || m_selectedIndex >= m_items.size())
        return nullptr;
    return qobject_cast<KFrame *>(m_items[m_selectedIndex]);
}

void KOsdSubMenu::RefreshSelectedItem()
{
    // Реф. @0x47ae30 — ЗАВОРАЧИВАНИЕ КУРСОРА ЖИВЁТ ЗДЕСЬ, а не в Up/DownKeyAct:
    //   if (m_selectedIndex >= count) m_selectedIndex = 0;
    //   else if (m_selectedIndex < 0) m_selectedIndex = count - 1;
    // затем setCurrentRow и проход по ВСЕМ строкам: текущая → Selected(), прочие →
    // UnSelected(); каждой строке обновляется sizeHint по фактической геометрии виджета.
    const int count = m_listWidget->count();
    if (m_selectedIndex >= count)
        m_selectedIndex = 0;
    else if (m_selectedIndex < 0)
        m_selectedIndex = count - 1;
    m_listWidget->setCurrentRow(m_selectedIndex);

    for (int i = 0; i < count; ++i) {
        QListWidgetItem *item = m_listWidget->item(i);
        QWidget *w = m_listWidget->itemWidget(item);
        if (!w)
            continue;
        if (KFrame *f = qobject_cast<KFrame *>(w)) {
            if (i == m_selectedIndex)
                f->Selected();     // реф. vt+0x1a8
            else
                f->UnSelected();   // реф. vt+0x1b0
        }
        // Реф.: sizeHint = QSize(right-left+1, bottom-top+1) фактической геометрии.
        item->setSizeHint(QSize(w->geometry().width(), w->geometry().height()));
    }
    m_listWidget->update();
}

void KOsdSubMenu::UpKeyAct()
{
    // Реф. @0x47b198: если текущая строка ЗАХВАТИЛА фокус (спин) — клавиша уходит ей,
    // курсор меню не двигается. Иначе --index и Refresh (он же и заворачивает).
    if (!isActiveWindow())
        return;
    KFrame *item = GetSelectedMenuItem();
    if (item && item->GotFocusIn()) {
        item->UpKeyAct();
        return;
    }
    --m_selectedIndex;
    RefreshSelectedItem();
}

void KOsdSubMenu::DownKeyAct()
{
    // Реф. @0x47b118 — зеркально UpKeyAct.
    if (!isActiveWindow())
        return;
    KFrame *item = GetSelectedMenuItem();
    if (item && item->GotFocusIn()) {
        item->DownKeyAct();
        return;
    }
    ++m_selectedIndex;
    RefreshSelectedItem();
}

void KOsdSubMenu::ItemClicked(const QModelIndex &idx)
{
    // Реф. @0x47b108 (ConfirmKeyAct(QModelIndex)): курсор = row, дальше обычный Confirm.
    if (!idx.isValid())
        return;
    m_selectedIndex = idx.row();
    RefreshSelectedItem();
}

void KOsdSubMenu::ConfirmKeyAct()
{
    // Реф. @0x47b080: гейт isActiveWindow(); item->Focused() (vt+0x1a0 — именно Focused,
    // НЕ Selected); item->ConfirmKeyAct(m_selectedIndex) (vt+0x1e8, аргумент — ИНДЕКС);
    // если item->IsSingleSelectLabel() (vt+0x1f0) → коммит checked + RefreshCheckedItem.
    if (!isActiveWindow())
        return;
    KFrame *f = GetSelectedMenuItem();
    if (!f)
        return;
    f->Focused();
    f->ConfirmKeyAct(m_selectedIndex);
    if (f->IsSingleSelectLabel()) {
        m_checkedIndex = m_selectedIndex;
        RefreshCheckedItem();
    }
}

void KOsdSubMenu::keyPressEvent(QKeyEvent *e)
{
    // Реф. @0x47b218: сначала «модификаторы нажаты → игнор», затем разбор кодов.
    if (e->modifiers() != Qt::NoModifier) {
        e->ignore();
        return;
    }
    switch (e->key()) {
    case Qt::Key_Up:                     // 0x1000013
        UpKeyAct();
        return;
    case Qt::Key_Down:                   // 0x1000015
        DownKeyAct();
        return;
    case Qt::Key_Right:                  // 0x1000014
    case Qt::Key_Return:                 // 0x1000004
    case Qt::Key_Enter:                  // 0x1000005
        ConfirmKeyAct();
        return;
    case Qt::Key_Left:                   // 0x1000012
    case Qt::Key_Escape: {               // 0x1000000
        // Реф.: «назад» = отпустить захваченный спин, а если фокус не захвачен — закрыть меню.
        KFrame *item = GetSelectedMenuItem();
        if (item && item->GotFocusIn()) {
            item->ReleaseFocus();
            item->Selected();
        } else {
            close();
        }
        return;
    }
    default:
        KOsdMenuBase::keyPressEvent(e);
        return;
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
    // Реф. @0x47b340: фан-аут в строку через ВИРТУАЛЬНЫЙ слот SetIntValue (vt+0x1f8) —
    // работает для любой KFrame-строки, а не только для спина.
    if (row < 0 || row >= m_items.size())
        return;
    if (KFrame *f = qobject_cast<KFrame *>(m_items[row]))
        f->SetIntValue(value);
}

void KOsdSubMenu::SetValue(int row, double value)
{
    if (row < 0 || row >= m_items.size())
        return;
    if (KOsdDoubleSpin *s = qobject_cast<KOsdDoubleSpin *>(m_items[row]))
        s->SetDoubleValue(value);
}
