#pragma once

#include "KOsdSingleSelectMenu.h"
#include <QPoint>

// Яркостный EQ — OSD single-select-подменю (реф. KImageBrightEQSetting : KOsdSingleSelectMenu,
// ctor @0x8146d0, UpdateVLSConfig). UI-порт. Список = KUserOsdSet::GetBrightEQList — числовые
// строки "0".."N-1", N = product.ini [Limit/BrightnessEQ] (per-model файл; в порте дефолт 4).
// msgType=0x11, msgParam=индекс. checkedIndex реф. = GetVideoParam()->GetBrightEQConfig()
// (device) — в порте дефолт 0. confirm-колбэк → SendToMainCtrl (DEVICE-seam, no-op).
class KImageBrightEQSetting : public KOsdSingleSelectMenu
{
    Q_OBJECT
public:
    explicit KImageBrightEQSetting(const QPoint &pos = QPoint(), QWidget *parent = nullptr);
};
