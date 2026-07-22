#include "KIrisSetting.h"

KIrisSetting::KIrisSetting(const QPoint &pos, int channel, QWidget *parent)
    : KOsdSubMenu(parent, /*bAddReturnBtn=*/true)   // реф. ctor: KOsdSubMenu(parent, true)
{
    setObjectName(QStringLiteral("KIrisSetting"));
    InitConfig(pos, channel);
}

void KIrisSetting::InitConfig(const QPoint &pos, int channel)
{
    // Реф. InitConfig + GetIrisList: 3 статические tr-ключа. msgType=0x1c, msgParam=channel.
    static const char *const keys[3] = {"TR_FScreen", "TR_WScreen", "TR_CScreen"};
    for (const char *k : keys) {
        KOsdSingleSelectLabelConfig cfg;
        cfg.text = tr(k);
        cfg.msgParam = channel;
        cfg.msgType = 0x1c;   // реф. msgType 28
        AddItem(cfg);
    }
    InitWidget(pos);
    // Реф. checkedIndex = GetVideoParam()->[+0x1c] (device) — в порте дефолт 0.
    InitCheckedItem(0);
}

void KIrisSetting::EnterMenu(const QPoint &pos, int channel, KOsdSubMenu *parentMenu)
{
    // Реф. @0x4786e0: new KIrisSetting → SetParentMenu → open. В порте — show().
    KIrisSetting *m = new KIrisSetting(pos, channel);
    if (parentMenu)
        m->SetParentMenu(parentMenu);
    m->show();
}
