#pragma once

#include "KOsdSubMenu.h"
#include <QPoint>

// Назначение функции физической кнопке — OSD-подменю (реф. KbuttonSetting : KOsdSubMenu,
// ctor @0x471908, InitConfig). UI-порт. bAddReturnBtn=true. Список = KUserOsdSet::
// GetFunctionNameList — СТАТИЧЕСКИЕ 12 tr-ключей (funcId 0..11), фильтр IsFuncEnbale
// (id1 IsZoomEnable / id10 IsSwitchVLSModeEnable / id11 IsVideoRecordEnable — в порте все true).
// Каждый = single-select-метка, msgType=0x2a, msgParam=keyIndex (какая кнопка). checkedIndex реф.
// = FunctionIdToIndex(GetButtonFunctionId(...)) — GetButtonFunctionId это user-config (device),
// в порте дефолт funcId 0 → index 0. Сигнал UserSetChange→UpdateCheckedItem — DEVICE-seam, опущен.
class KbuttonSetting : public KOsdSubMenu
{
    Q_OBJECT
public:
    explicit KbuttonSetting(const QPoint &pos = QPoint(), int keyIndex = 1, QWidget *parent = nullptr);

public slots:
    void UpdateCheckedItem(int a, int b) { Q_UNUSED(a); InitCheckedItem(b); }   // реф.: игнор a → InitCheckedItem(b)

private:
    void InitConfig(const QPoint &pos, int keyIndex);
};
