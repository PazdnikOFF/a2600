#include "KIrisMenu.h"

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
    AddItem(KOsdLabelConfig{ tr("TR_OpMode"), nullptr });   // реф. → KOperationModeSetting::EnterMenu
    AddItem(KOsdLabelConfig{ tr("TR_IRIS1"), nullptr });    // реф. → KIrisSetting::EnterMenu
    InitWidget(pos);
}
