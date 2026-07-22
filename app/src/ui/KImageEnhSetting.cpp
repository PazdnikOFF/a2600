#include "KImageEnhSetting.h"

KImageEnhSetting::KImageEnhSetting(const QPoint &pos, int channel, QWidget *parent)
    : KOsdSubMenu(parent, /*bAddReturnBtn=*/true)   // реф. ctor: KOsdSubMenu(parent, true)
{
    setObjectName(QStringLiteral("KImageEnhSetting"));
    InitConfig(pos, channel);
    // Реф. ctor-side: SetImageEnhMode(channel+1)/SendSetImageEnhLevel/SetImageEnhConfig — device, опущено.
}

void KImageEnhSetting::InitConfig(const QPoint &pos, int channel)
{
    // Реф. InitConfig @0x4763d8: inline "L1"/"L2"/"L3" (литералы, НЕ tr). msgType=0xc, msgParam=channel.
    for (int i = 0; i < 3; ++i) {
        KOsdSingleSelectLabelConfig cfg;
        cfg.text = QStringLiteral("L") + QString::number(i + 1);   // "L1".."L3"
        cfg.msgParam = channel;
        cfg.msgType = 0x0c;   // реф. msgType 12
        AddItem(cfg);
    }
    InitWidget(pos);
    // Реф. checkedIndex = GetVideoParam()->GetImgEnhLevel(channel) (device) — в порте дефолт 0.
    InitCheckedItem(0);
}
