#pragma once

#include "KOsdSubMenu.h"
#include <QPoint>

// Определение пользовательских кнопок — OSD-подменю (реф. KButtonDefinitionMenu : KOsdSubMenu,
// ctor @0x471298, InitConfig @0x470e10). UI-порт. Тонкий фабричный подкласс: bAddReturnBtn=true;
// InitConfig строит 5 строк (TR_PMButton/HDAButton/PAButton/HDBButton/PBButton), у всех единый
// action = KbuttonSetting::EnterMenu (открытие под-подменю) — DEVICE-seam, в порте no-op.
// NeedToCloseWhenCameraDisconnected()→false — совпадает с базой (реф. переопределяет тем же
// значением; в порте не дублируем).
class KButtonDefinitionMenu : public KOsdSubMenu
{
    Q_OBJECT
public:
    explicit KButtonDefinitionMenu(const QPoint &pos = QPoint(), QWidget *parent = nullptr);

private:
    void InitConfig(const QPoint &pos);   // реф. @0x470e10
};
