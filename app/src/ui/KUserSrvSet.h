#pragma once

#include "ui/KDialog.h"

// Диалог сервисных настроек (реф. KUserSrvSet : KDialog, ctor @0x70c0b8,
// Ui_KUserSrvSet::setupUi @0x710158). UI-порт. Немодальный, 460×768, SetKStyle(W460),
// титул SetTitle(TR_Svce). Две группы + Exit:
//   • groupBoxServer (TR_Fnctn): секции Log(Backup/View), MMControl(Processor/Endo),
//     Otr(Upgrade/Recovery + videoCal/lightConfig) — кнопки открывают под-диалоги;
//   • groupBoxInfo (TR_EInformaion): 5 строк caption/value (SN процессора/наработка лампы
//     тек.-всего/SN эндоскопа/число использований — значения device);
//   • frameButton: центрированный Exit (fixed 120).
// Все stock Qt, кастомов нет.
//
// DEVICE в порт не тянется: слоты кнопок (ClickProcessorCtl/EndoCtl/BackupLog/ViewLog/
// Recovery/Upgrade/LightConfig/VideoCal), proxy-подписки статусов, TimerTaskRecovery,
// InitWidget (5 info-меток + роль-гейтинг btn_lightConfig role<3) — заглушки. Exit→close.
class KUserSrvSet : public KDialog
{
    Q_OBJECT
public:
    explicit KUserSrvSet(QWidget *parent = nullptr);

private:
    void setupUi();
};
