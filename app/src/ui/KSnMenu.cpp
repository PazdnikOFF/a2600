#include "KSnMenu.h"

KSnMenu::KSnMenu(const QPoint &pos, QWidget *parent)
    : KOsdSubMenu(parent, /*bAddReturnBtn=*/true)   // реф. ctor: KOsdSubMenu(parent, true)
{
    setObjectName(QStringLiteral("KSnMenu"));
    InitConfig(pos);
}

void KSnMenu::InitConfig(const QPoint &pos)
{
    // Реф. @0x47caa8: 2 read-only инфо-строки (action=NULL). Реф. дописывает значение из
    // GetCamera()->GetCameraInfo() (модель/серийник) — DEVICE, в порте только tr-подпись.
    AddItem(KOsdLabelConfig{ tr("TR_Mdl:"), nullptr });   // + модель камеры (device)
    AddItem(KOsdLabelConfig{ tr("TR_SN:"), nullptr });    // + серийник камеры (device)
    InitWidget(pos);
}
