#include "KImageBrightEQSetting.h"

KImageBrightEQSetting::KImageBrightEQSetting(const QPoint &pos, QWidget *parent)
    : KOsdSingleSelectMenu(parent)
{
    setObjectName(QStringLiteral("KImageBrightEQSetting"));

    // Реф. UpdateVLSConfig: GetBrightEQList = "0".."N-1", N = product.ini [Limit/BrightnessEQ]
    // (per-model, device-config) — в порте дефолт 4. msgType=0x11, msgParam=индекс.
    const int kBrightEQLevel = 4;   // реф. KProjectSet::GetBrightEQLevel() (product.ini) — дефолт
    QList<Item> list;
    for (int i = 0; i < kBrightEQLevel; ++i) {
        KOsdSingleSelectLabelConfig cfg;
        cfg.text = QString::number(i);
        cfg.msgParam = i;
        cfg.msgType = 0x11;   // реф. msgType 17
        list.append(qMakePair(cfg, KOsdSingleSelectLabel::ConfirmCallback()));   // seam no-op
    }
    // Реф. checkedIndex = GetVideoParam()->GetBrightEQConfig() (device) — в порте дефолт 0.
    InitConfig(pos, list, /*checkedIndex=*/0);
}
