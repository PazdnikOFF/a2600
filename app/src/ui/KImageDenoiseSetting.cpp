#include "KImageDenoiseSetting.h"

KImageDenoiseSetting::KImageDenoiseSetting(const QPoint &pos, QWidget *parent)
    : KOsdSingleSelectMenu(parent)
{
    setObjectName(QStringLiteral("KImageDenoiseSetting"));

    // Реф. UpdateVLSConfig: GetImgDenoiseList = "0".."N-1", N = product.ini [Limit/ImgDenoise]
    // (per-model, device-config) — в порте дефолт 4. msgType=0x10, msgParam=индекс.
    const int kDenoiseLevel = 4;   // реф. KProjectSet::GetImgDenoiseLevel() (product.ini) — дефолт
    QList<Item> list;
    for (int i = 0; i < kDenoiseLevel; ++i) {
        KOsdSingleSelectLabelConfig cfg;
        cfg.text = QString::number(i);
        cfg.msgParam = i;
        cfg.msgType = 0x10;   // реф. msgType 16
        list.append(qMakePair(cfg, KOsdSingleSelectLabel::ConfirmCallback()));   // seam no-op
    }
    // Реф. checkedIndex = GetVideoParam()->GetDenoiseConfig() (device) — в порте дефолт 0.
    InitConfig(pos, list, /*checkedIndex=*/0);
}
