#pragma once

#include "KOsdSubMenu.h"
#include <QPoint>

// Диафрагма (iris) режим-экрана — OSD-подменю (реф. KIrisSetting : KOsdSubMenu, ctor @0x478500,
// InitConfig). UI-порт. Открывается из KIrisMenu (строка TR_IRIS1) через EnterMenu. bAddReturnBtn=
// true. 3 tr-строки TR_FScreen/WScreen/CScreen (KUserOsdSet::GetIrisList — статические),
// msgParam=channel, msgType=0x1c; выбор=индекс. checkedIndex реф. = GetVideoParam()->[+0x1c]
// (device→деф.0). VideoParamChangeActImpl (a==7→InitCheckedItem) — device re-sync, в порте no-op.
class KIrisSetting : public KOsdSubMenu
{
    Q_OBJECT
public:
    explicit KIrisSetting(const QPoint &pos = QPoint(), int channel = 0, QWidget *parent = nullptr);

    // Реф. фабрика @0x4786e0: открытие под-подменю из строки KIrisMenu (TR_IRIS1).
    static void EnterMenu(const QPoint &pos, int channel, KOsdSubMenu *parentMenu);

private:
    void InitConfig(const QPoint &pos, int channel);
};
