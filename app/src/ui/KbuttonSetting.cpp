#include "KbuttonSetting.h"

KbuttonSetting::KbuttonSetting(const QPoint &pos, int keyIndex, QWidget *parent)
    : KOsdSubMenu(parent, /*bAddReturnBtn=*/true)   // реф. ctor: KOsdSubMenu(parent, true)
{
    setObjectName(QStringLiteral("KbuttonSetting"));
    InitConfig(pos, keyIndex);
    // Реф. connect(GetSystemStatus()::UserSetChange → UpdateCheckedItem) — DEVICE-seam, опущено.
}

void KbuttonSetting::InitConfig(const QPoint &pos, int keyIndex)
{
    // Реф. InitConfig: GetFunctionNameList = СТАТИЧЕСКИЕ 12 tr-ключей (funcId порядок), фильтр
    // IsFuncEnbale. Гейты id1/id10/id11 (IsZoomEnable/IsSwitchVLSModeEnable/IsVideoRecordEnable) —
    // device-config, в порте все true (показываем полный список). msgType=0x2a, msgParam=keyIndex.
    static const char *const keys[12] = {
        "TR_Frz", "TR_Zm1", "TR_IRIS1", "TR_AGC1", "TR_IEnh", "TR_Snp",
        "TR_Brtnss+", "TR_Brtnss-", "TR_Ctrst", "TR_WBalance", "TR_LMode", "TR_Rcd"
    };
    for (const char *k : keys) {
        KOsdSingleSelectLabelConfig cfg;
        cfg.text = tr(k);
        cfg.msgParam = keyIndex;   // реф. какая физическая кнопка
        cfg.msgType = 0x2a;        // реф. msgType 42
        AddItem(cfg);
    }
    InitWidget(pos);
    // Реф. checkedIndex = FunctionIdToIndex(GetButtonFunctionId(IndexToKeyID(keyIndex)-0x213)).
    // GetButtonFunctionId — user-config (device); дефолт funcId 0 → index 0.
    InitCheckedItem(0);
}
