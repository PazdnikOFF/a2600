#pragma once

#include "KOsdSingleSelectMenu.h"
#include <QPoint>

// Зум — OSD single-select-подменю (реф. KZoomSetting : KOsdSingleSelectMenu, ctor @0x47e618,
// GetConfigs). UI-порт. 3 строки "L1"/"L2"/"L3" (ЛИТЕРАЛЫ, НЕ tr — inline в GetConfigs),
// msgType=8, msgParam=ранг 0/1/2. checkedIndex реф. = GetVideoParam()->[+0x20] (device→деф.0).
// VideoParamChangeActImpl (реф. a==6→InitCheckedItem) — device re-sync, в порте no-op.
class KZoomSetting : public KOsdSingleSelectMenu
{
    Q_OBJECT
public:
    explicit KZoomSetting(const QPoint &pos = QPoint(), QWidget *parent = nullptr);
};
