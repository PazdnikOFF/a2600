#pragma once

#include "ui/KDialog.h"

// Диалог настройки параметров алгоритма /算法参数调试 (реф. KAlgParamAjustDlg : KDialog,
// ctor @0x5c9a80, Ui_KAlgParamAjustDlg::setupUi @0x5cced0). UI-порт. Немодальный, 330×900,
// SetKStyle(W460), титул 算法参数调试. Подписи — китайский+ASCII инлайн-tr() (fromUtf8).
// QStackedWidget 2 страницы: page1 = SCL(特殊光)/master-switch/CCM/AWB(白平衡); page2 =
// Knee/Gamma/Denoise(降噪)/BrightBalance(亮度均衡) + ряд чекбоксов фич (SensorLUT/色彩增强/
// 图像增强/电子去烟/宽动态) + Exit/> + конвертер定点浮点转换. Много hex-QSpinBox
// (setDisplayIntegerBase(16)). Все stock Qt, кастомов нет.
//
// pb_nextPage переключает страницы; конвертер fixed↔float — чистая арифметика (реализованы).
// DEVICE в порт не тянется: Slot_checkBox_*/Slot_pb_*_save (push в FPGA/KPlControl),
// InitStatus (device-pull чек-состояний), Slot_endo_switch, 2 QTimer-поллера — заглушки.
// Exit→close.
class KAlgParamAjustDlg : public KDialog
{
    Q_OBJECT
public:
    explicit KAlgParamAjustDlg(QWidget *parent = nullptr);

private:
    void setupUi();
};
