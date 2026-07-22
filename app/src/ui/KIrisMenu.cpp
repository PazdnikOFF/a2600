#include "KIrisMenu.h"
#include "KOperationModeSetting.h"
#include "KIrisSetting.h"

KIrisMenu::KIrisMenu(const QPoint &pos, QWidget *parent)
    : KOsdSubMenu(parent, /*bAddReturnBtn=*/true)   // реф. ctor: KOsdSubMenu(parent, true)
{
    setObjectName(QStringLiteral("KIrisMenu"));
    InitConfig(pos);
}

void KIrisMenu::InitConfig(const QPoint &pos)
{
    // Реф. @0x4777c0: 2 строки-метки. action = Xxx::EnterMenu (открытие под-подменю) —
    // DEVICE-seam, под-подменю (KOperationModeSetting/KIrisSetting) не портированы → no-op.
    // TR_OpMode → KOperationModeSetting (портирован); action открывает под-подменю.
    AddItem(KOsdLabelConfig{ tr("TR_OpMode"),
        [](QPoint p, int, KOsdSubMenu *) { (new KOperationModeSetting(p))->show(); } });
    // TR_IRIS1 → KIrisSetting (портирован); action открывает под-подменю.
    AddItem(KOsdLabelConfig{ tr("TR_IRIS1"),
        [](QPoint p, int ch, KOsdSubMenu *m) { KIrisSetting::EnterMenu(p, ch, m); } });
    InitWidget(pos);
}
