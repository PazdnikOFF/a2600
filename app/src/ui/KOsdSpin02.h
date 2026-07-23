#pragma once

#include "KOsdSpinBase.h"
#include "KOsdSpin.h"   // _KOsdSpinConfig (у нас KOsdSpinConfig) — общий конфиг обоих спинов

class QGridLayout;
class QLabel;
class QSpinBox;

// РАБОЧИЙ спин строки OSD-подменю (реф. KOsdSpin02 : KOsdSpinBase, ctor @0x484e18,
// sizeof 0x78; setupUi заинлайнен в ctor).
//
// ⭐ ГЛАВНЫЙ ФАКТ (сверено по всему .text): в прошивке НЕТ НИ ОДНОГО вызова ctor KOsdSpin
// @0x484050, ни ссылки на его vtable @0xa29e98 — **KOsdSpin мёртвый код**, а единственный
// реально создаваемый спин — ЭТОТ (KOsdSubMenu::AddItem(_KOsdSpinConfig) @0x47bd20).
// Поэтому KOsdSpin в порте оставлен как есть (историческая раскладка с кнопками ±),
// а хостится теперь KOsdSpin02.
//
// Раскладка (все константы из ctor): фрейм 250×32 фикс (min=max=250×32), sizePolicy
// Fixed/Fixed, objectName "KOsdSpin02"; QGridLayout "gridLayout" hspacing 6, vspacing 0,
// margins (9,0,9,0); label_title в (0,0); spin_value в (0,1) — QSpinBox, min-width 180,
// NoFocus, autoFillBackground(false), styleSheet(""), wrapping(false), AlignHCenter|
// AlignVCenter (0x84) и ⚠️ setButtonSymbols(UpDownArrows) — НАТИВНЫЕ стрелки видны,
// в отличие от KOsdSpin (NoButtons + свои кнопки «+»/«−»).
//
// DEVICE-seam: ValueChangedAct шлёт KUiMsgProxy::SendToMainCtrl(msgType, msgParam, value) —
// заменён Qt-сигналом valueChanged(int); msgType/msgParam хранятся как данные.
class KOsdSpin02 : public KOsdSpinBase
{
    Q_OBJECT
public:
    explicit KOsdSpin02(const KOsdSpinConfig &cfg, QWidget *parent = nullptr);

    int Value() const { return m_cur; }
    void SetIntValue(int v) override;   // реф. @0x485500 — тихий сеттер через флаг m_notify

    // Для проверок/превью (в реф. — просто члены Ui-структуры).
    QLabel *TitleLabel() const { return m_title; }
    QSpinBox *SpinBox() const { return m_spin; }

signals:
    void valueChanged(int value);   // порт: замена device SendToMainCtrl

public:
    // Реф. @0x484ba8/@0x484c50/@0x484cf8: база KFrame + доп. stylesheet на QSpinBox.
    void Focused() override;
    void Selected() override;
    void UnSelected() override;

protected:
    void ConfirmKeyActImpl(int index) override;   // реф. @0x485448 — ПУСТО (только ret)
    void AddValue() override;                     // реф. @0x4854a8: cur+=step, клип по max
    void SubValue() override;                     // реф. @0x485450: cur-=step, клип по min

private slots:
    void ValueChangedAct(int value);   // реф. @0x485538 (приватный слот, гейт m_notify)

private:
    void InitWidget();   // реф. @0x484da0

    KOsdSpinConfig m_cfg;
    QGridLayout *m_grid = nullptr;
    QLabel *m_title = nullptr;
    QSpinBox *m_spin = nullptr;
    int m_cur = 0;              // +0x5c (инициализируется cfg.def)
    bool m_notify = true;       // +0x70
};
