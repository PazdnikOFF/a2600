#include "KImageEnhTypeSetting.h"

KImageEnhTypeSetting::KImageEnhTypeSetting(const QPoint &pos, QWidget *parent)
    : KOsdSubMenu(parent, /*bAddReturnBtn=*/true)   // реф. ctor: KOsdSubMenu(parent, true)
{
    setObjectName(QStringLiteral("KImageEnhTypeSetting"));
    InitConfig(pos);
}

void KImageEnhTypeSetting::InitConfig(const QPoint &pos)
{
    // Реф. InitConfig: 3 KOsdLabel, единый action = KImageEnhTypeSetting::EnterMenu (под-подменю)
    // — DEVICE-seam, в порте no-op (nullptr).
    AddItem(KOsdLabelConfig{ tr("TR_StreA"), nullptr });
    AddItem(KOsdLabelConfig{ tr("TR_StreB"), nullptr });
    AddItem(KOsdLabelConfig{ tr("TR_Ege2"), nullptr });
    InitWidget(pos);
    // Реф. checkedIndex = GetVideoParam()->ImagEnhMode()-1 (device) — в порте дефолт 0.
    InitCheckedItem(0);
}
