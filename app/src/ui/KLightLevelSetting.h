#pragma once

#include "KOsdSubMenu.h"
#include <QPoint>

// Уровень света — OSD-подменю (реф. KLightLevelSetting : KOsdSubMenu, ctor @0x478a30, InitConfig).
// UI-порт. bAddReturnBtn=true. 1 KOsdSpin "L" (ЛИТЕРАЛ), диапазон 1..19 шаг 1, def реф. =
// GetSystemStatus()[+0x4c] (device→деф.1), msgId=0, ctxId=31. SystemStatusChangeActImpl (param
// 0xd→SetValue) — device re-sync, в порте no-op.
class KLightLevelSetting : public KOsdSubMenu
{
    Q_OBJECT
public:
    explicit KLightLevelSetting(const QPoint &pos = QPoint(), QWidget *parent = nullptr);
    static void EnterMenu(const QPoint &pos, int, KOsdSubMenu *parentMenu);

private:
    void InitConfig(const QPoint &pos);
};
