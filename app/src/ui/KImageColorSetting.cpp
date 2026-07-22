#include "KImageColorSetting.h"

KImageColorSetting::KImageColorSetting(const QPoint &pos, QWidget *parent)
    : KOsdSubMenu(parent, /*bAddReturnBtn=*/true)
{
    setObjectName(QStringLiteral("KImageColorSetting"));
    InitConfig(pos);
    // Реф. QTimer::singleShot(0, CheckToClose) — device auto-close, опущено.
}

void KImageColorSetting::InitConfig(const QPoint &pos)
{
    // Реф. InitConfig @0x4757a8: 3 KOsdSpin "R"/"B"/"C" (литералы), -15..15 шаг 1.
    // def = GetVideoParam() color-регистры (device) → дефолт 0. msgId=i, ctxId=26.
    const char *const titles[3] = {"R", "B", "C"};
    for (int i = 0; i < 3; ++i) {
        KOsdSpinConfig cfg;
        cfg.title = QString::fromLatin1(titles[i]);
        cfg.min = -15; cfg.max = 15; cfg.step = 1; cfg.def = 0;
        cfg.msgId = i; cfg.ctxId = 26;
        AddItem(cfg);
    }
    InitWidget(pos);
}

void KImageColorSetting::EnterMenu(const QPoint &pos, int, KOsdSubMenu *parentMenu)
{
    // Реф. @0x475da8: new KImageColorSetting → SetParentMenu → open.
    KImageColorSetting *m = new KImageColorSetting(pos);
    if (parentMenu)
        m->SetParentMenu(parentMenu);
    m->show();
}
