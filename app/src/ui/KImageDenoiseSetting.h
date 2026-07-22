#pragma once

#include "KOsdSingleSelectMenu.h"
#include <QPoint>

// Шумоподавление — OSD single-select-подменю (реф. KImageDenoiseSetting : KOsdSingleSelectMenu,
// ctor @0x815248, UpdateVLSConfig). UI-порт. Идентичен KImageBrightEQSetting: список =
// KUserOsdSet::GetImgDenoiseList = "0".."N-1", N = product.ini [Limit/ImgDenoise] (в порте деф. 4).
// msgType=0x10, msgParam=индекс. checkedIndex = GetVideoParam()->GetDenoiseConfig() (device→деф.0).
class KImageDenoiseSetting : public KOsdSingleSelectMenu
{
    Q_OBJECT
public:
    explicit KImageDenoiseSetting(const QPoint &pos = QPoint(), QWidget *parent = nullptr);
};
