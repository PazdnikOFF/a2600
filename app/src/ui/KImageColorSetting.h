#pragma once

#include "KOsdSubMenu.h"
#include <QPoint>

// Цвет (R/B/C) — OSD-подменю (реф. KImageColorSetting : KOsdSubMenu, ctor @0x475bb8, InitConfig).
// UI-порт. bAddReturnBtn=true. 3 KOsdSpin-строки "R"/"B"/"C" (ЛИТЕРАЛЫ), диапазон -15..15 шаг 1,
// def реф. = GetVideoParam() color-регистры (device→деф.0), msgId=0/1/2, ctxId=26. Ctor-таймер
// CheckToClose + AllowedToOpen(SystemStatus!=3) + VideoParamChangeActImpl — device, опущены/no-op.
class KImageColorSetting : public KOsdSubMenu
{
    Q_OBJECT
public:
    explicit KImageColorSetting(const QPoint &pos = QPoint(), QWidget *parent = nullptr);
    static void EnterMenu(const QPoint &pos, int, KOsdSubMenu *parentMenu);

private:
    void InitConfig(const QPoint &pos);
};
