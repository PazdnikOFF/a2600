#include "KButtonDefinitionMenu.h"

KButtonDefinitionMenu::KButtonDefinitionMenu(const QPoint &pos, QWidget *parent)
    : KOsdSubMenu(parent, /*bAddReturnBtn=*/true)   // реф. ctor: KOsdSubMenu(parent, true)
{
    setObjectName(QStringLiteral("KButtonDefinitionMenu"));
    InitConfig(pos);
}

void KButtonDefinitionMenu::InitConfig(const QPoint &pos)
{
    // Реф. @0x470e10: 5 строк, у всех единый action = KbuttonSetting::EnterMenu (открытие
    // под-подменю) — DEVICE-seam, в порте no-op (под-подменю не портировано).
    static const char *const keys[5] = {
        "TR_PMButton", "TR_HDAButton", "TR_PAButton", "TR_HDBButton", "TR_PBButton"
    };
    for (const char *k : keys)
        AddItem(KOsdLabelConfig{ tr(k), nullptr });
    InitWidget(pos);
}
