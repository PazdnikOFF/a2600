#include "KOsdSpin.h"

#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <QSpinBox>

KOsdSpin::KOsdSpin(const KOsdSpinConfig &cfg, QWidget *parent)
    : QFrame(parent)
    , m_cfg(cfg)
{
    // Реф. ctor @0x484050: setupUi → InitWidget → connect (3 сигнала).
    setupUi(cfg);
    InitWidget();
    connect(m_btnAdd, &QPushButton::clicked, this, &KOsdSpin::ClickAddBtnAct);
    connect(m_btnSub, &QPushButton::clicked, this, &KOsdSpin::ClickSubBtnAct);
    connect(m_spin, QOverload<int>::of(&QSpinBox::valueChanged), this, &KOsdSpin::ValueChangedAct);
}

void KOsdSpin::setupUi(const KOsdSpinConfig &cfg)
{
    // Реф. Ui_KOsdSpin::setupUi @0x484330.
    setObjectName(QStringLiteral("KOsdSpin"));
    setMinimumWidth(250);
    setMaximumWidth(250);   // фикс-ширина 250 (min=max=0xfa)

    QGridLayout *g = new QGridLayout(this);
    g->setContentsMargins(9, 0, 9, 0);

    m_title = new QLabel(cfg.title, this);
    m_title->setObjectName(QStringLiteral("label_title"));
    g->addWidget(m_title, 0, 0);

    // spacer col1 — схлопывается в 0×0 при пустом заголовке.
    g->addItem(new QSpacerItem(cfg.title.isEmpty() ? 0 : 10, 0,
                               QSizePolicy::Expanding, QSizePolicy::Minimum), 0, 1);

    m_btnSub = new QPushButton(QStringLiteral("-"), this);
    m_btnSub->setObjectName(QStringLiteral("btn_sub"));
    m_btnSub->setFixedSize(30, 30);
    m_btnSub->setFocusPolicy(Qt::NoFocus);
    g->addWidget(m_btnSub, 0, 2);

    m_spin = new QSpinBox(this);
    m_spin->setObjectName(QStringLiteral("spin_value"));
    m_spin->setMinimumWidth(120);
    m_spin->setFocusPolicy(Qt::NoFocus);
    m_spin->setAlignment(Qt::AlignCenter);
    m_spin->setButtonSymbols(QAbstractSpinBox::NoButtons);   // native ± скрыты
    g->addWidget(m_spin, 0, 3);

    m_btnAdd = new QPushButton(QStringLiteral("+"), this);
    m_btnAdd->setObjectName(QStringLiteral("btn_add"));
    m_btnAdd->setFixedSize(30, 30);
    m_btnAdd->setFocusPolicy(Qt::NoFocus);
    g->addWidget(m_btnAdd, 0, 4);
}

void KOsdSpin::InitWidget()
{
    // Реф. @0x483f98: применить конфиг к внутреннему спину.
    m_spin->setMinimum(m_cfg.min);
    m_spin->setMaximum(m_cfg.max);
    m_spin->setSingleStep(m_cfg.step);
    m_spin->setValue(m_cfg.def);
    m_cur = m_cfg.def;
    RefreshSpinStatus();
}

void KOsdSpin::RefreshSpinStatus()
{
    // Реф. @0x483ef0: клип к границам, enable/disable ±кнопок, обновить спин.
    if (m_cur < m_cfg.min) m_cur = m_cfg.min;
    if (m_cur > m_cfg.max) m_cur = m_cfg.max;
    m_btnSub->setEnabled(m_cur > m_cfg.min);
    m_btnAdd->setEnabled(m_cur < m_cfg.max);
    m_spin->setValue(m_cur);
}

void KOsdSpin::ClickAddBtnAct()
{
    m_cur += m_cfg.step;
    RefreshSpinStatus();
}

void KOsdSpin::ClickSubBtnAct()
{
    m_cur -= m_cfg.step;
    RefreshSpinStatus();
}

void KOsdSpin::ValueChangedAct(int value)
{
    // Реф.: SendToMainCtrl(msgId, ctxId, value) — DEVICE. Порт: Qt-сигнал.
    m_cur = value;
    emit valueChanged(value);
}

void KOsdSpin::SetIntValue(int v)
{
    // Реф. @0x484250: тихий сеттер — отключить notify, задать, подключить обратно.
    disconnect(m_spin, QOverload<int>::of(&QSpinBox::valueChanged), this, &KOsdSpin::ValueChangedAct);
    m_cur = v;
    m_spin->setValue(v);
    RefreshSpinStatus();
    connect(m_spin, QOverload<int>::of(&QSpinBox::valueChanged), this, &KOsdSpin::ValueChangedAct);
}
