#pragma once

#include "KOsdSubMenu.h"
#include <QPoint>

// Тип усиления изображения — OSD-подменю (реф. KImageEnhTypeSetting : KOsdSubMenu, ctor @0x815990,
// InitConfig). UI-порт. Тонкая фабрика: bAddReturnBtn=true; 3 KOsdLabel (TR_StreA/TR_StreB/TR_Ege2),
// единый action=EnterMenu (под-подменю, no-op в порте); InitWidget + InitCheckedItem(текущий тип).
// checkedIndex реф. = GetVideoParam()->ImagEnhMode()-1 (device video-param) — в порте дефолт 0.
class KImageEnhTypeSetting : public KOsdSubMenu
{
    Q_OBJECT
public:
    explicit KImageEnhTypeSetting(const QPoint &pos = QPoint(), QWidget *parent = nullptr);

private:
    void InitConfig(const QPoint &pos);
};
