#pragma once

#include "ui/KDialog.h"

// Диалог монитора температуры /温度监测 (реф. KSystemTemperature : KDialog, ctor @0x5f9658,
// Ui_KSystemTemperature::setupUi @0x5f9c38). UI-порт. Немодальный, 400×336, SetKStyle(W460),
// титул 温度监测. Подписи — китайский инлайн-tr() (fromUtf8). Состав: frame → сетка 3
// строк caption/value (4EV温度/白灯温度/UV灯温度, значения device) + Exit. Все stock Qt,
// кастомов нет.
//
// DEVICE в порт не тянется: UpdateTemperature (QTimer 1с: KPlControl::ReadValueFromPL
// 0xffa50a14 + ColdLight::QueryLightTemperature) — заглушка; значения пусты. Exit→close
// (в реф. btn_exit не подключён — подключаем для юзабилити).
class KSystemTemperature : public KDialog
{
    Q_OBJECT
public:
    explicit KSystemTemperature(QWidget *parent = nullptr);

private:
    void setupUi();
};
