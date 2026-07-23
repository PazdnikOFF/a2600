#pragma once

#include "ui/KDialog.h"

#include <QString>

// Диалог ввода серийника процессора (реф. KProcessorSN : KDialog, ctor @0x6ff748,
// Ui_KProcessorSN::setupUi @0x700068). UI-порт. Сиблинг KEndoScopeSN. Немодальный,
// 640×480, SetKStyle(W460), титул TR_PSN. Центрирующая сетка со спейсерами: заголовок
// TR_PITPSN: + строка SN:/lineEdit (validator [A-Za-z0-9]{0,12}) + OK/Cancel (фикс 100).
// Все stock Qt. Диалог ЦЕЛИКОМ pure-UI: onOK — валидация (>8 симв.) + сохранение в m_SN,
// getter GetSetSN(); device-запись SN в процессор — в ВЫЗЫВАЮЩЕМ коде, не здесь.
// Cancel→close.
class KProcessorSN : public KDialog
{
    Q_OBJECT
public:
    explicit KProcessorSN(QWidget *parent = nullptr);

    // Реф. GetSetSN(): введённый серийный номер отдаётся ВЫЗЫВАЮЩЕМУ (запись в процессор —
    // его дело). Читает OpenProcessorSNSetDlg @0x6fff40.
    QString GetSetSN() const;

private:
    void setupUi();
};
