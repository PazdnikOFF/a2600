#include "KOsdSingleSelectMenu.h"

KOsdSingleSelectMenu::KOsdSingleSelectMenu(QWidget *parent)
    : KOsdSubMenu(parent, /*bAddReturnBtn=*/true)   // реф. ctor: KOsdSubMenu(parent, true)
{
    setObjectName(QStringLiteral("KOsdSingleSelectMenu"));
}

void KOsdSingleSelectMenu::InitConfig(const QPoint &pos, const QList<Item> &items, int checkedIndex)
{
    // Реф. InitConfig @0x483970: на каждую пару {cfg, action} — new KOsdSingleSelectLabel + AddItem,
    // затем InitWidget(pos) + InitCheckedItem(checkedIndex).
    for (const Item &it : items)
        AddItem(new KOsdSingleSelectLabel(it.first, this, it.second));
    InitWidget(pos);
    InitCheckedItem(checkedIndex);
}
