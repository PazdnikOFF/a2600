#include "KContrastSetting.h"

KContrastSetting::KContrastSetting(const QPoint &pos, QWidget *parent)
    : KOsdSingleSelectMenu(parent)
{
    setObjectName(QStringLiteral("KContrastSetting"));

    // Реф. GetConfigs: 3 статические строки + ранг {0,1,2}. msgType=0xe.
    const char *const keys[3] = {"TR_Low", "TR_Mdle", "TR_Hgh"};
    QList<Item> list;
    for (int i = 0; i < 3; ++i) {
        KOsdSingleSelectLabelConfig cfg;
        cfg.text = tr(keys[i]);
        cfg.msgParam = i;      // реф. _CONTRAST_RANK
        cfg.msgType = 0x0e;    // реф. msgType 14
        list.append(qMakePair(cfg, KOsdSingleSelectLabel::ConfirmCallback()));   // seam no-op
    }
    // Реф. checkedIndex = GetVideoParam()->[+0x3c] (device) — в порте дефолт 0.
    InitConfig(pos, list, /*checkedIndex=*/0);
}
