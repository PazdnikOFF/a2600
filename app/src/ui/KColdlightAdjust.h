#pragma once

#include "ui/KDialog.h"

// Диалог настройки холодного источника света (реф. KColdlightAdjust : KDialog,
// ctor @0x747f68, Ui_KColdlightAdjust::setupUi @0x74a300). UI-порт. Немодальный,
// 450×1161, SetKStyle(W460), титул SetTitle(TR_LSConfiguration). Это сервис/тюнинг
// источника: (1) groupBox_2 (TR_LLTest) — редактор времени работы лампы (часы/минуты +
// Save + суммарное время); (2) groupBox_automatic_dimmer_param — PID-параметры авто-
// диммера (pAgc/iAgc/dAgc/pAlc/iAlc/dAlc/pAec/iAec/dAec Up/Down double-спины dec3 step
// 0.001, agc max/min/threshold/pwm/delt* int/double спины, + самотест диммера: чекбоксы
// start/random, комбо AGC/AEC, flickTh/range/fixStep); (3) groupBox (TR_AIris) — живой
// вывод замеров автоапертуры (7 device-меток); (4) frame — Save/Exit. Всё stock Qt,
// кастомов нет.
//
// DEVICE в порт не тянется: SaveAutomaticDimmerParam/SaveLightUseTime, RefreahData
// (периодическое обновление меток по QTimer), DimmingTest* (аппаратный самотест),
// EndoScope/Camera/SystemStatus-сигналы, все value-метки — заглушки. Exit→close.
class KColdlightAdjust : public KDialog
{
    Q_OBJECT
public:
    explicit KColdlightAdjust(QWidget *parent = nullptr);

private:
    void setupUi();
};
