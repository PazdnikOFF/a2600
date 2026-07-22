#pragma once

#include "KOsdSubMenu.h"
#include <QPoint>

// Усиление изображения (уровень) — OSD-подменю (реф. KImageEnhSetting : KOsdSubMenu,
// ctor @0x476778, InitConfig). UI-порт. bAddReturnBtn=true. 3 single-select-строки-ЛИТЕРАЛА
// "L1"/"L2"/"L3" (inline, НЕ tr), msgParam=channel, msgType=0xc; выбор=индекс. checkedIndex реф.
// = GetVideoParam()->GetImgEnhLevel(channel) (device→деф.0). ctor-side SetImageEnhMode/
// SendSetImageEnhLevel/SetImageEnhConfig + AllowedToOpen(IsImageEnhEnable) + VideoParamChangeActImpl
// — device, в порте опущены/no-op.
class KImageEnhSetting : public KOsdSubMenu
{
    Q_OBJECT
public:
    explicit KImageEnhSetting(const QPoint &pos = QPoint(), int channel = 0, QWidget *parent = nullptr);

private:
    void InitConfig(const QPoint &pos, int channel);
};
