#include "KOsdDoubleSpin.h"

#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <QDoubleSpinBox>

KOsdDoubleSpin::KOsdDoubleSpin(const KOsdDoubleSpinConfig &cfg, QWidget *parent)
    : QFrame(parent)
    , m_cfg(cfg)
{
    // Реф. ctor @0x47ff98: setupUi → InitWidget → connect.
    setupUi(cfg);
    InitWidget();
    connect(m_btnAdd, &QPushButton::clicked, this, &KOsdDoubleSpin::ClickAddBtnAct);
    connect(m_btnSub, &QPushButton::clicked, this, &KOsdDoubleSpin::ClickSubBtnAct);
    connect(m_spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &KOsdDoubleSpin::ValueChangedAct);
}

void KOsdDoubleSpin::setupUi(const KOsdDoubleSpinConfig &cfg)
{
    // Идентично KOsdSpin, но внутренний виджет — QDoubleSpinBox.
    setObjectName(QStringLiteral("KOsdDoubleSpin"));
    setMinimumWidth(250);
    setMaximumWidth(250);

    QGridLayout *g = new QGridLayout(this);
    g->setContentsMargins(9, 0, 9, 0);

    m_title = new QLabel(cfg.title, this);
    m_title->setObjectName(QStringLiteral("label_title"));
    g->addWidget(m_title, 0, 0);

    g->addItem(new QSpacerItem(cfg.title.isEmpty() ? 0 : 10, 0,
                               QSizePolicy::Expanding, QSizePolicy::Minimum), 0, 1);

    m_btnSub = new QPushButton(QStringLiteral("-"), this);
    m_btnSub->setObjectName(QStringLiteral("btn_sub"));
    m_btnSub->setFixedSize(30, 30);
    m_btnSub->setFocusPolicy(Qt::NoFocus);
    g->addWidget(m_btnSub, 0, 2);

    m_spin = new QDoubleSpinBox(this);
    m_spin->setObjectName(QStringLiteral("doubleSpinBox"));
    m_spin->setMinimumWidth(120);
    m_spin->setFocusPolicy(Qt::NoFocus);
    m_spin->setAlignment(Qt::AlignCenter);
    m_spin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_spin->setLayoutDirection(Qt::LeftToRight);   // реф. явный вызов
    g->addWidget(m_spin, 0, 3);

    m_btnAdd = new QPushButton(QStringLiteral("+"), this);
    m_btnAdd->setObjectName(QStringLiteral("btn_add"));
    m_btnAdd->setFixedSize(30, 30);
    m_btnAdd->setFocusPolicy(Qt::NoFocus);
    g->addWidget(m_btnAdd, 0, 4);
}

void KOsdDoubleSpin::InitWidget()
{
    m_spin->setDecimals(m_cfg.decimals);
    m_spin->setMinimum(m_cfg.min);
    m_spin->setMaximum(m_cfg.max);
    m_spin->setSingleStep(m_cfg.step);
    m_spin->setValue(m_cfg.def);
    m_cur = m_cfg.def;
    RefreshSpinStatus();
}

void KOsdDoubleSpin::RefreshSpinStatus()
{
    if (m_cur < m_cfg.min) m_cur = m_cfg.min;
    if (m_cur > m_cfg.max) m_cur = m_cfg.max;
    m_btnSub->setEnabled(m_cur > m_cfg.min);
    m_btnAdd->setEnabled(m_cur < m_cfg.max);
    m_spin->setValue(m_cur);
}

void KOsdDoubleSpin::ClickAddBtnAct()
{
    m_cur += m_cfg.step;
    RefreshSpinStatus();
}

void KOsdDoubleSpin::ClickSubBtnAct()
{
    m_cur -= m_cfg.step;
    RefreshSpinStatus();
}

void KOsdDoubleSpin::ValueChangedAct(double value)
{
    // Реф.: SendToMainCtrl (int)(value*10+offset) — DEVICE. Порт: Qt-сигнал double.
    m_cur = value;
    emit valueChanged(value);
}

void KOsdDoubleSpin::SetDoubleValue(double v)
{
    disconnect(m_spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
               this, &KOsdDoubleSpin::ValueChangedAct);
    m_cur = v;
    m_spin->setValue(v);
    RefreshSpinStatus();
    connect(m_spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &KOsdDoubleSpin::ValueChangedAct);
}
