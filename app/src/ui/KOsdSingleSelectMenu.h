#pragma once

#include "KOsdSubMenu.h"
#include "KOsdSingleSelectLabel.h"
#include <QList>
#include <QPair>
#include <QPoint>

// Single-select OSD-меню (реф. KOsdSingleSelectMenu : KOsdSubMenu, ctor @0x483c08,
// InitConfig @0x483970). UI-порт. Тонкий фабричный подкласс: вся машинерия — в KOsdSubMenu
// (курсор/чек-индекс/RefreshCheckedItem/ConfirmKeyAct), строки — KOsdSingleSelectLabel.
// InitConfig(pos, список пар {cfg, action-колбэк}, checkedIndex): на каждую пару строит
// KOsdSingleSelectLabel и хостит через AddItem, затем InitWidget(pos) + InitCheckedItem.
// bAddReturnBtn=true (реф. ctor). DEVICE-seam — только confirm-колбэк строки (см. label).
class KOsdSingleSelectMenu : public KOsdSubMenu
{
    Q_OBJECT
public:
    using Item = QPair<KOsdSingleSelectLabelConfig, KOsdSingleSelectLabel::ConfirmCallback>;

    explicit KOsdSingleSelectMenu(QWidget *parent = nullptr);

    void InitConfig(const QPoint &pos, const QList<Item> &items, int checkedIndex);
};
