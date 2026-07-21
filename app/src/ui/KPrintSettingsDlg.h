#pragma once

#include "ui/KDialog.h"

// Диалог настроек печати (реф. KPrintSettingsDlg : KDialog, ctor @0x78ec10,
// Ui_KPrintSettingsDlg::setupUi @0x78f970). UI-порт. Немодальный, 900×900, SetKStyle НЕТ,
// титул SetTitle(TR_PESettings) + тёмный setStyleSheet. Состав:
//   • m_pImageLbl — превью снимка (device, RenderPixmapOfImageLabel) → плейсхолдер;
//   • сетка принтера: имя принтера (device-комбо) + размер бумаги (A4/Carta статично) +
//     чекбокс оптимизации TR_IOptimization;
//   • сетка слайдеров: яркость [-100,100] и гамма [0,2000] (KMySlider→QSlider) с
//     кнопками −/+ (25×25);
//   • ряд кнопок OK/Cancel/LoadDefault.
//
// Кастом KMySlider→QSlider (horizontal, ticksBothSides). OK/Cancel — чистый UI (close);
// слайдеры/±/LoadDefault/printer-change/превью — device (гамма/яркость печати +
// KPrinterManager). Paper-combo — статичные пункты.
class KPrintSettingsDlg : public KDialog
{
    Q_OBJECT
public:
    explicit KPrintSettingsDlg(QWidget *parent = nullptr);

private:
    void setupUi();
};
