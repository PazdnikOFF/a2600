#pragma once

#include "KOsdSingleSelectMenu.h"
#include <QPoint>

// VLS-режим (спектральные комбо) — OSD single-select-подменю (реф. KVlsModeSetting :
// KOsdSingleSelectMenu, ctor @0x47d990, UpdateVLSConfig). UI-порт. Список — из per-продукт
// coldlight.ini через KColdLightConfig::GetVLSConfigDisplayList (комбо режимов WL/EWL/SFI/VIST,
// NULL-слоты убраны, склейка "->"). msgType=6, msgParam=VLS-enum (в порте — индекс комбо).
// checkedIndex реф. = FindTheItemIndex(GetSystemStatus current VLS_MODE) (device→деф.0).
// SystemStatusChangeActImpl (param 9→re-sync) — device, в порте no-op. FindTheItemIndex — pure.
class KVlsModeSetting : public KOsdSingleSelectMenu
{
    Q_OBJECT
public:
    explicit KVlsModeSetting(const QPoint &pos = QPoint(), QWidget *parent = nullptr);

    // Реф. @0x47d348: линейный скан по msgParam == mode, иначе def. Чистая (портируем as-is).
    static int FindTheItemIndex(const QList<Item> &items, int vlsMode, int def);
};
