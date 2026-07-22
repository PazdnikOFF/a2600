#pragma once

#include "KOsdSubMenu.h"
#include <QPoint>

// Iris/aperture OSD-подменю (реф. KIrisMenu : KOsdSubMenu, ctor @0x477a10, InitConfig @0x4777c0).
// UI-порт. Тонкий фабричный подкласс: bAddReturnBtn=true; InitConfig строит 2 строки-метки
// (TR_OpMode → KOperationModeSetting::EnterMenu, TR_IRIS1 → KIrisSetting::EnterMenu) через
// KOsdSubMenu::AddItem(KOsdLabelConfig) и зовёт InitWidget(pos). action-колбэки (открытие
// под-подменю) — DEVICE-seam, в порте no-op (под-подменю не портированы).
class KIrisMenu : public KOsdSubMenu
{
    Q_OBJECT
public:
    explicit KIrisMenu(const QPoint &pos = QPoint(), QWidget *parent = nullptr);

private:
    void InitConfig(const QPoint &pos);   // реф. @0x4777c0
};
