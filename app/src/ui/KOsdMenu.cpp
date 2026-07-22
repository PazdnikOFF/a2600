#include "KOsdMenu.h"
#include "KOsdMenuCell.h"
#include "KOsdRootMenuItems.h"

#include <QVBoxLayout>
#include <QListWidget>
#include <QKeyEvent>

// ── KOsdMenuBase ─────────────────────────────────────────────────────────────

KOsdMenuBase::KOsdMenuBase(QWidget *parent)
    : KDialog(parent, false)
{
    // Реф. ctor @0x47a780: только коннекты device-сигналов (KUiMsgProxy/SystemStatus/Camera/
    // VideoParam) в слоты *Impl — опущено как device. Виджеты не строятся.
}

int KOsdMenuBase::GetOsdMenuLevel() const
{
    // Реф.: глубина по цепочке m_parentMenu (root=1, +1 за предка).
    int level = 1;
    const KOsdMenuBase *p = m_parentMenu;
    while (p) { ++level; p = p->m_parentMenu; }
    return level;
}

void KOsdMenuBase::keyPressEvent(QKeyEvent *e)
{
    // Реф.: только F12 → ExitAllMenus; прочее базе.
    if (e->key() == Qt::Key_F12) {
        ExitAllMenus();
        return;
    }
    KDialog::keyPressEvent(e);
}

// ── KOsdMenu ─────────────────────────────────────────────────────────────────

KOsdMenu::KOsdMenu(QWidget *parent)
    : KOsdMenuBase(parent)
{
    // Реф. ctor @0x479ed0: layout + listWidget + стили + безрамочное translucent-окно.
    setObjectName(QStringLiteral("KOsdMenu"));

    QWidget *content = ContentArea();
    m_layout = new QVBoxLayout(content);
    m_layout->setObjectName(QStringLiteral("verticalLayout"));
    m_layout->setContentsMargins(0, 0, 0, 0);

    m_listWidget = new QListWidget(content);
    m_listWidget->setObjectName(QStringLiteral("listWidget"));
    m_listWidget->setFocusPolicy(Qt::NoFocus);
    m_listWidget->setStyleSheet(QStringLiteral(
        "QListWidget{outline:none; border:none; background:transparent;}"
        "QListWidget::item::selected{background:transparent;}"));
    m_layout->addWidget(m_listWidget);

    // Реф. InitConnect: KUiMsgProxy OsdMenuUp/Down/Confirm → слоты (device-источник опущен);
    // клик по списку → ItemClicked.
    connect(m_listWidget, &QListWidget::clicked, this, &KOsdMenu::ItemClicked);
}

void KOsdMenu::AddItem(KOsdMenuCell *cell)
{
    // Реф. @0x479740: back-pointer + greyed + insertItem + setItemWidget.
    if (!cell)
        return;
    cell->SetLocatedMenu(this);
    cell->UpdateGreyedFlag();
    QListWidgetItem *item = new QListWidgetItem();
    item->setSizeHint(cell->sizeHint());
    m_listWidget->insertItem(m_listWidget->count(), item);
    m_listWidget->setItemWidget(item, cell);
    m_cells.append(cell);
    if (m_cells.size() == 1)
        RefreshMenu(-1);   // первая ячейка → выбрать
}

void KOsdMenu::InitWidget()
{
    // Реф. @0x479c70: фикс-порядок корневых ячеек, затем AddItem каждой + setCurrentRow(0).
    // KRecordItem (#2) — DEFERRED (device-heavy: system-status/USB/record); реф. гейт
    // KProjectSet::IsVideoRecordEnable() — в порте пропускаем (поведение при record off).
    AddItem(new KIrisItem(this));            // #1 TR_Mode → KIrisMenu
    AddItem(new KimageProcesItem(this));     // #3 TR_IParameters → KImageProcessingMenu (deferred)
    AddItem(new KExitItem(this));            // #4 TR_Ext → close
    AddItem(new KFeaturesItem(this));        // #5 TR_Fnctn → KFeatureMenu (deferred)
    AddItem(new KButtonDefineItem(this));    // #6 TR_Button → KButtonDefinitionMenu
    AddItem(new KCameraInfoItem(this));      // #7 TR_Camera → KSnMenu
    m_currentIndex = 0;
    RefreshMenu(-1);
}

void KOsdMenu::RefreshMenu(int oldIdx)
{
    // Реф.: старая строка UnSelect, новая Select + setCurrentRow.
    if (oldIdx >= 0 && oldIdx < m_cells.size())
        m_cells[oldIdx]->UnSelect();
    m_listWidget->setCurrentRow(m_currentIndex);
    if (m_currentIndex >= 0 && m_currentIndex < m_cells.size()) {
        m_cells[m_currentIndex]->Select();
        if (QListWidgetItem *it = m_listWidget->item(m_currentIndex))
            it->setSizeHint(m_cells[m_currentIndex]->sizeHint());
    }
    update();
}

void KOsdMenu::UpKeyAct()
{
    if (m_cells.isEmpty())
        return;
    const int old = m_currentIndex;
    m_currentIndex = (m_currentIndex - 1 < 0) ? m_cells.size() - 1 : m_currentIndex - 1;
    RefreshMenu(old);
}

void KOsdMenu::DownKeyAct()
{
    if (m_cells.isEmpty())
        return;
    const int old = m_currentIndex;
    m_currentIndex = (m_currentIndex + 1 >= m_cells.size()) ? 0 : m_currentIndex + 1;
    RefreshMenu(old);
}

void KOsdMenu::ItemClicked(const QModelIndex &idx)
{
    if (!idx.isValid())
        return;
    const int old = m_currentIndex;
    m_currentIndex = idx.row();
    RefreshMenu(old);
}

void KOsdMenu::ConfirmKeyAct()
{
    // Реф. @ConfirmKeyAct: текущая ячейка → Focus + SetSubWindowPosition + активация (virtual).
    if (m_currentIndex < 0 || m_currentIndex >= m_cells.size())
        return;
    KOsdMenuCell *cell = m_cells[m_currentIndex];
    cell->Focus();
    cell->SetSubWindowPosition(pos() + m_subPos);
    cell->ConfirmAct();   // реф. virtual cell-activate: открытие подменю/close (подкласс)
}
