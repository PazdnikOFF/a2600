#include "KFeatureMenu.h"

KFeatureMenu::KFeatureMenu(const QPoint &pos, QWidget *parent)
    : KOsdSubMenu(parent, /*bAddReturnBtn=*/true)
{
    setObjectName(QStringLiteral("KFeatureMenu"));
    InitConfig(pos);
}

void KFeatureMenu::InitConfig(const QPoint &pos)
{
    // Реф. InitConfig @0x474128: 7 KOsdStatusLabel (title tr + value ""). value заполняет
    // UpdateStatus() (device) → в порте пусто. msgType по таблице реверса.
    struct { const char *title; int msgType; } items[7] = {
        {"TR_AGC1", 7}, {"TR_Frz", 9}, {"TR_Moire", 0x1e}, {"TR_WBalance", 0x0d},
        {"TR_SRemoval", 0x12}, {"HDR", 0x13}, {"TR_Tmr", 0x1025}
    };
    for (const auto &it : items) {
        KOsdStatusLabelConfig cfg;
        cfg.title = tr(it.title);
        cfg.value = QString();   // реф. пусто; device UpdateStatus заполняет
        cfg.msgType = it.msgType;
        AddItem(cfg);
    }
    InitWidget(pos);
    // Реф. GetMenuItem(2) TR_Moire → SetGreyed(!IsDemoireEnable()) (device) — в порте не серо.
}

void KFeatureMenu::EnterMenu(const QPoint &pos, int, KOsdSubMenu *parentMenu)
{
    KFeatureMenu *m = new KFeatureMenu(pos);
    if (parentMenu)
        m->SetParentMenu(parentMenu);
    m->show();
}
