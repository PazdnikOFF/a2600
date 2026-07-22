#include "KZoomSetting.h"

KZoomSetting::KZoomSetting(const QPoint &pos, QWidget *parent)
    : KOsdSingleSelectMenu(parent)
{
    setObjectName(QStringLiteral("KZoomSetting"));

    // Реф. GetConfigs (inline): "L1"/"L2"/"L3" (литералы, НЕ tr) + ранг {0,1,2}. msgType=8.
    QList<Item> list;
    for (int i = 0; i < 3; ++i) {
        KOsdSingleSelectLabelConfig cfg;
        cfg.text = QStringLiteral("L") + QString::number(i + 1);   // "L1".."L3"
        cfg.msgParam = i;      // реф. _ZOOM_RANK
        cfg.msgType = 0x08;    // реф. msgType 8
        list.append(qMakePair(cfg, KOsdSingleSelectLabel::ConfirmCallback()));   // seam no-op
    }
    // Реф. checkedIndex = GetVideoParam()->[+0x20] (device) — в порте дефолт 0.
    InitConfig(pos, list, /*checkedIndex=*/0);
}
