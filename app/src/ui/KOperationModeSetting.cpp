#include "KOperationModeSetting.h"

KOperationModeSetting::KOperationModeSetting(const QPoint &pos, int channel, QWidget *parent)
    : KOsdSubMenu(parent, /*bAddReturnBtn=*/true)   // реф. ctor: KOsdSubMenu(parent, true)
{
    setObjectName(QStringLiteral("KOperationModeSetting"));
    InitConfig(pos, channel);
}

void KOperationModeSetting::InitConfig(const QPoint &pos, int channel)
{
    // Реф. InitConfig + GetOperationModeList: 10 статических tr-ключей, у всех msgParam=channel,
    // msgType=0x1d; выбор = индекс строки. Хостинг single-select-строк через AddItem(config).
    static const char *const keys[10] = {
        "TR_LMode1", "TR_TMode", "TR_HMode", "TR_Cmode", "TR_AMode",
        "TR_SMode", "TR_TMode1", "TR_Umode", "TR_Rmode1", "TR_FMode"
    };
    for (const char *k : keys) {
        KOsdSingleSelectLabelConfig cfg;
        cfg.text = tr(k);
        cfg.msgParam = channel;   // реф. video-param канал (const по строкам)
        cfg.msgType = 0x1d;       // реф. msgType 29
        AddItem(cfg);
    }
    InitWidget(pos);
    // Реф. checkedIndex = GetVideoParam()->[+0x18] (device) — в порте дефолт 0.
    InitCheckedItem(0);
}
