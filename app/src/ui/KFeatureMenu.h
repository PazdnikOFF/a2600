#pragma once

#include "KOsdSubMenu.h"
#include <QPoint>

// Функции (статусы) — OSD-подменю (реф. KFeatureMenu : KOsdSubMenu, ctor @0x4748c0, InitConfig).
// UI-порт. bAddReturnBtn=true. 7 KOsdStatusLabel-строк (заголовок + значение справа); значения
// реф. заполняет UpdateStatus() (device) → в порте пусто "". Строка TR_Moire реф. сереет по
// !IsDemoireEnable() (device) — в порте не серо. Video/SystemStatusChangeActImpl — device
// re-sync, в порте no-op.
class KFeatureMenu : public KOsdSubMenu
{
    Q_OBJECT
public:
    explicit KFeatureMenu(const QPoint &pos = QPoint(), QWidget *parent = nullptr);
    static void EnterMenu(const QPoint &pos, int, KOsdSubMenu *parentMenu);

private:
    void InitConfig(const QPoint &pos);
};
