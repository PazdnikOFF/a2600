#pragma once

#include "KFrame.h"
#include <QString>
#include <functional>

class QLabel;

// Конфиг строки single-select-меню (реф. _KOsdSingleSelectLabelConfig, 16 байт):
// текст + пара msg-полей, что уходят в SendToMainCtrl при подтверждении.
struct KOsdSingleSelectLabelConfig
{
    QString text;
    int msgParam = 0;   // +0x8
    int msgType = 0;    // +0xc
};

// Строка single-select OSD-меню (реф. KOsdSingleSelectLabel : KFrame, ctor @0x582c30, size 0x78).
// UI-порт. QHBoxLayout(margins 9,0,0,0) + label_icon(QLabel) + текст(QLabel), фикс-ширина 250.
// Состояние «выбрано» — СМЕНА ИКОНКИ (не подсветка): Checked→singlechoice_select.png,
// UnChecked→singlechoice_normal.png (реф. KDisplayOption::GetOsdIconPixmap → theme black/osd).
// IsSingleSelectLabel()→true (реф. база KFrame→false). DEVICE-SEAM: ConfirmKeyAct(value) реф.
// зовёт стратегию ConfirmAction→KUiMsgProxy::SendToMainCtrl(msgType,msgParam,value); в порте —
// инъектируемый std::function (по умолчанию no-op). 100% PORT (кроме seam-колбэка).
class KOsdSingleSelectLabel : public KFrame
{
    Q_OBJECT
public:
    // Реф. ConfirmAction-стратегия → инъектируемый колбэк (cfg + value). Дефолт — no-op (device-seam).
    using ConfirmCallback = std::function<void(const KOsdSingleSelectLabelConfig &, int value)>;

    explicit KOsdSingleSelectLabel(const KOsdSingleSelectLabelConfig &cfg,
                                   QWidget *parent = nullptr,
                                   ConfirmCallback action = nullptr);

    void Checked() override;                    // реф. @0x583218: select-иконка
    void UnChecked() override;                  // реф. @0x5832f0: normal-иконка
    void ConfirmKeyAct(int value) override;     // реф. @0x582bf0: action(cfg, value) — DEVICE-seam
    bool IsSingleSelectLabel() const override { return true; }   // реф. @0x482c18

    const KOsdSingleSelectLabelConfig &GetConfig() const { return m_cfg; }

private:
    QPixmap osdIcon(const QString &name) const;   // DEVICE-STUB KDisplayOption::GetOsdIconPixmap

    KOsdSingleSelectLabelConfig m_cfg;
    ConfirmCallback m_action;
    QLabel *m_labelIcon = nullptr;   // Ui[1]
    QLabel *m_labelText = nullptr;   // Ui[2]
};
