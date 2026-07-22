#pragma once

#include "KOsdSingleSelectMenu.h"
#include <QPoint>

// Контраст — OSD single-select-подменю (реф. KContrastSetting : KOsdSingleSelectMenu,
// ctor @0x472718, GetConfigs). UI-порт. 3 СТАТИЧЕСКИЕ строки TR_Low/TR_Mdle/TR_Hgh
// (KUserOsdSet::GetContrastList), msgType=0xe, msgParam=ранг 0/1/2. checkedIndex реф. =
// GetVideoParam()->[+0x3c] (device→деф.0). VideoParamChangeActImpl (реф. a==0x17→InitCheckedItem)
// — device re-sync, в порте no-op (база). confirm-колбэк → SendToMainCtrl (DEVICE-seam, no-op).
class KContrastSetting : public KOsdSingleSelectMenu
{
    Q_OBJECT
public:
    explicit KContrastSetting(const QPoint &pos = QPoint(), QWidget *parent = nullptr);
};
