#pragma once

#include "KOsdSubMenu.h"
#include <QPoint>

// Режим работы — OSD-подменю (реф. KOperationModeSetting : KOsdSubMenu, ctor @0x4790d0,
// InitConfig). UI-порт. Открывается из KIrisMenu (строка TR_OpMode). bAddReturnBtn=true.
// 10 СТАТИЧЕСКИХ single-select-строк (KUserOsdSet::GetOperationModeList), у всех msgParam=channel
// (ctor-арг, video-param канал), msgType=0x1d; выбор кодируется ИНДЕКСОМ строки (0..9).
// checkedIndex реф. = GetVideoParam()->[+0x18] (device→деф.0). VideoParamChangeActImpl
// (реф. a==0x18→InitCheckedItem(b)) — device re-sync, в порте no-op.
class KOperationModeSetting : public KOsdSubMenu
{
    Q_OBJECT
public:
    explicit KOperationModeSetting(const QPoint &pos = QPoint(), int channel = 0, QWidget *parent = nullptr);

private:
    void InitConfig(const QPoint &pos, int channel);
};
