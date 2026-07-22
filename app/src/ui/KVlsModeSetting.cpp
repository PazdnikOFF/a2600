#include "KVlsModeSetting.h"
#include "ctrl/KColdLightConfig.h"
#include "sys/KSystem.h"

#include <QDir>

KVlsModeSetting::KVlsModeSetting(const QPoint &pos, QWidget *parent)
    : KOsdSingleSelectMenu(parent)
{
    setObjectName(QStringLiteral("KVlsModeSetting"));

    // Реф. UpdateVLSConfig @0x47d3c0: имена из KColdLightConfig::GetVLSConfigDisplayList (комбо
    // спектральных режимов; NULL убраны; склейка "->"). Грузим per-продукт coldlight.ini (как
    // self-test coldlight). msgType=6, msgParam=индекс комбо (реф. VLSModeNameToEnum).
    KColdLightConfig &cl = KColdLightConfig::GetInstance();
    if (cl.VLSConfigNum() == 0) {
        const QString path = QDir(KSystem::SystemPath())
                                 .absoluteFilePath(QStringLiteral("coldlight/X-2600/X-2600B/coldlight.ini"));
        cl.LoadVLSConfig(path);
    }
    const QVector<QStringList> combos = cl.GetVLSConfigDisplayList();

    QList<Item> list;
    for (int i = 0; i < combos.size(); ++i) {
        KOsdSingleSelectLabelConfig cfg;
        cfg.text = combos[i].join(QStringLiteral("->"));   // реф. склейка режимов "->"
        cfg.msgParam = i;      // реф. VLSModeNameToEnum (в порте — индекс)
        cfg.msgType = 6;
        list.append(qMakePair(cfg, KOsdSingleSelectLabel::ConfirmCallback()));   // seam no-op
    }
    // Реф. checkedIndex = FindTheItemIndex(GetSystemStatus current VLS_MODE) — device → деф.0.
    InitConfig(pos, list, /*checkedIndex=*/0);
}

int KVlsModeSetting::FindTheItemIndex(const QList<Item> &items, int vlsMode, int def)
{
    // Реф. @0x47d348: скан по msgParam == vlsMode.
    for (int i = 0; i < items.size(); ++i)
        if (items[i].first.msgParam == vlsMode)
            return i;
    return def;
}
