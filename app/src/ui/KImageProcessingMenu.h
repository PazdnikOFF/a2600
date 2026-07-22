#pragma once

#include "KOsdSubMenu.h"
#include <QPoint>

// Параметры изображения — OSD-подменю-ХАБ (реф. KImageProcessingMenu : KOsdSubMenu, ctor
// @0x477248, InitConfig). UI-порт. bAddReturnBtn=true. 8 KOsdLabel-строк (БЕЗ сепараторов),
// каждая открывает своё под-подменю настройки. Строка TR_Zm1 гейтится IsZoomEnable (в порте
// показываем), Enh/Col реф. сереют по AllowedToOpen (в порте не серо). Video/SystemStatus-хуки
// — device re-sync, no-op. Открывается из корневого KOsdMenu (KimageProcesItem).
class KImageProcessingMenu : public KOsdSubMenu
{
    Q_OBJECT
public:
    explicit KImageProcessingMenu(const QPoint &pos = QPoint(), QWidget *parent = nullptr);
    static void EnterMenu(const QPoint &pos, int, KOsdSubMenu *parentMenu);

private:
    void InitConfig(const QPoint &pos);
};
