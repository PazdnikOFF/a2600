#pragma once

#include "KFrame.h"
#include <QString>

class QLabel;

// Конфиг строки-статуса OSD-меню (реф. _KOsdStatusLabelConfig, 0x18 байт): заголовок + значение
// + msgType (id устройства). Реверс подтвердил: НЕТ action-fn-ptr — только int msgType.
struct KOsdStatusLabelConfig
{
    QString title;
    QString value;
    int msgType = 0;   // +0x10 (device routing)
};

// Строка-статус OSD-меню (реф. KOsdStatusLabel : KFrame, ctor @0x4857f8, size 0x70). UI-порт.
// QHBoxLayout(spacing 0, margins 9,0,9,0): label_title | spacer(expanding) | label_status,
// фикс 250×32. Заголовок слева, значение справа. Иконок НЕТ. SetStringValue → label_status.
// DEVICE-SEAM: ConfirmKeyAct(key) реф. зовёт KUiMsgProxy::SendToMainCtrl(msgType) (1-арг) —
// в порте no-op (msgType хранится). 100% PORT (кроме seam).
class KOsdStatusLabel : public KFrame
{
    Q_OBJECT
public:
    explicit KOsdStatusLabel(const KOsdStatusLabelConfig &cfg, QWidget *parent = nullptr);

    void InitWidget();                       // реф.: title/status.setText из конфига
    void SetStringValue(const QString &v);   // реф.: label_status.setText(v)
    void ConfirmKeyAct(int key) override;    // реф.: SendToMainCtrl(m_msgType) — DEVICE-seam (no-op)

private:
    QString m_title;                 // +0x58
    QString m_value;                 // +0x60
    int m_msgType = 0;               // +0x68
    QLabel *m_labelTitle = nullptr;  // Ui[1]
    QLabel *m_labelStatus = nullptr; // Ui[3]
};
