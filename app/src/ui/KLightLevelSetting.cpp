#include "KLightLevelSetting.h"

KLightLevelSetting::KLightLevelSetting(const QPoint &pos, QWidget *parent)
    : KOsdSubMenu(parent, /*bAddReturnBtn=*/true)
{
    setObjectName(QStringLiteral("KLightLevelSetting"));
    InitConfig(pos);
}

void KLightLevelSetting::InitConfig(const QPoint &pos)
{
    // Реф. InitConfig @0x4788c0: 1 KOsdSpin "L" (литерал), 1..19 шаг 1. def = GetSystemStatus()
    // [+0x4c] (device) → дефолт 1 (min). msgId=0, ctxId=31.
    KOsdSpinConfig cfg;
    cfg.title = QStringLiteral("L");
    cfg.min = 1; cfg.max = 19; cfg.step = 1; cfg.def = 1;
    cfg.msgId = 0; cfg.ctxId = 31;
    AddItem(cfg);
    InitWidget(pos);
}

void KLightLevelSetting::EnterMenu(const QPoint &pos, int, KOsdSubMenu *parentMenu)
{
    // Реф. @0x478c08: new KLightLevelSetting → SetParentMenu → open.
    KLightLevelSetting *m = new KLightLevelSetting(pos);
    if (parentMenu)
        m->SetParentMenu(parentMenu);
    m->show();
}
