#include "KImageProcessingMenu.h"
#include "KLightLevelSetting.h"
#include "KZoomSetting.h"
#include "KImageEnhTypeSetting.h"
#include "KContrastSetting.h"
#include "KImageColorSetting.h"
#include "KImageDenoiseSetting.h"
#include "KImageBrightEQSetting.h"

KImageProcessingMenu::KImageProcessingMenu(const QPoint &pos, QWidget *parent)
    : KOsdSubMenu(parent, /*bAddReturnBtn=*/true)
{
    setObjectName(QStringLiteral("KImageProcessingMenu"));
    InitConfig(pos);
}

void KImageProcessingMenu::InitConfig(const QPoint &pos)
{
    // Реф. InitConfig @0x476bd0: 8 KOsdLabel, каждая action = Xxx::EnterMenu (открытие под-подменю).
    // В порте action = лямбда, конструирующая целевое (портированное) подменю. TR_LMode →
    // KVlsModeSetting (НЕ портирован) → nullptr. Реф.-гейты Zoom/Enh/Col (device) — показываем всё.
    AddItem(KOsdLabelConfig{ tr("TR_Brtnss1"),
        [](QPoint p, int, KOsdSubMenu *) { (new KLightLevelSetting(p))->show(); } });
    AddItem(KOsdLabelConfig{ tr("TR_Zm1"),
        [](QPoint p, int, KOsdSubMenu *) { (new KZoomSetting(p))->show(); } });
    AddItem(KOsdLabelConfig{ tr("TR_Enh"),
        [](QPoint p, int, KOsdSubMenu *) { (new KImageEnhTypeSetting(p))->show(); } });
    AddItem(KOsdLabelConfig{ tr("TR_Ctrst"),
        [](QPoint p, int, KOsdSubMenu *) { (new KContrastSetting(p))->show(); } });
    AddItem(KOsdLabelConfig{ tr("TR_Col"),
        [](QPoint p, int, KOsdSubMenu *) { (new KImageColorSetting(p))->show(); } });
    AddItem(KOsdLabelConfig{ tr("TR_LMode"), nullptr });   // реф. → KVlsModeSetting (DEFERRED, config-driven)
    AddItem(KOsdLabelConfig{ tr("TR_IDenoise"),
        [](QPoint p, int, KOsdSubMenu *) { (new KImageDenoiseSetting(p))->show(); } });
    AddItem(KOsdLabelConfig{ tr("TR_BEquilibria"),
        [](QPoint p, int, KOsdSubMenu *) { (new KImageBrightEQSetting(p))->show(); } });
    InitWidget(pos);
}

void KImageProcessingMenu::EnterMenu(const QPoint &pos, int, KOsdSubMenu *parentMenu)
{
    KImageProcessingMenu *m = new KImageProcessingMenu(pos);
    if (parentMenu)
        m->SetParentMenu(parentMenu);
    m->show();
}
