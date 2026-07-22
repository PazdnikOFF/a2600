#pragma once

#include "KOsdSubMenu.h"
#include <QPoint>

// SN/модель OSD-подменю (реф. KSnMenu : KOsdSubMenu, ctor @0x47cef8, InitConfig @0x47caa8).
// UI-порт. Тонкий фабричный подкласс: bAddReturnBtn=true; InitConfig строит 2 инфо-строки
// (TR_Mdl: + модель камеры, TR_SN: + серийник) через AddItem(KOsdLabelConfig), action=NULL
// (read-only), затем InitWidget(pos). Значения модели/серийника реф. добавляет из
// GetCamera()->GetCameraInfo() (DEVICE) — в порте опущены (только tr-подписи).
class KSnMenu : public KOsdSubMenu
{
    Q_OBJECT
public:
    explicit KSnMenu(const QPoint &pos = QPoint(), QWidget *parent = nullptr);

private:
    void InitConfig(const QPoint &pos);   // реф. @0x47caa8
};
