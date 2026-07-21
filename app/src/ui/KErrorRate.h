#pragma once

#include "ui/KDialog.h"

// Диалог теста частоты битовых ошибок /误码率测试 (реф. KErrorRate : KDialog, ctor
// @0x5e1768, Ui_KErrorRate::setupUi @0x5e4a28). UI-порт. QC-диалог. Немодальный, 284×728,
// SetKStyle(W460), титул 误码率测试. Подписи — инлайн-tr() с китайским/ASCII текстом (НЕ
// TR_-ключи); у нас через fromUtf8. Состав: группа «结果» (результат, device) + чекбокс
// «出错后停止调试» + параметры теста (интервал/время h+m/счётчики) + группа «阈值»
// (пороги risk/NG) + группа «错误统计» (register/Lane0/Lane1/CRC/ECC/SOT, device) +
// кнопки 开始/停止/暂停/继续 (min120×30). Все stock Qt, кастомов нет.
//
// DEVICE в порт не тянется: SlotStart/Stop/Pause/Continue (BER-тест процессора),
// Slot_checkBox, Slot_timeOut/CalResult (QTimer-тик, счётчики ошибок Lane/CRC/ECC/SOT),
// все value-метки — заглушки. Стейт-машина enable кнопок — чистый UI, реализована.
class KErrorRate : public KDialog
{
    Q_OBJECT
public:
    explicit KErrorRate(QWidget *parent = nullptr);

private:
    void setupUi();
};
