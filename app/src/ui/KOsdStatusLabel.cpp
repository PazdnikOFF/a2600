#include "KOsdStatusLabel.h"

#include <QHBoxLayout>
#include <QLabel>

KOsdStatusLabel::KOsdStatusLabel(const KOsdStatusLabelConfig &cfg, QWidget *parent)
    : KFrame(parent)
    , m_title(cfg.title)
    , m_value(cfg.value)
    , m_msgType(cfg.msgType)
{
    // Реф. ctor @0x4857f8 + setupUi: QHBoxLayout(0, margins 9,0,9,0) = title | spacer | status,
    // фикс 250×32. proxy = GetKUiMsgProxy() (device, опущено). InitWidget заполняет тексты.
    setObjectName(QStringLiteral("KOsdStatusLabel"));
    setFixedSize(250, 32);   // реф. min=max=resize (0xfa, 0x20)

    QHBoxLayout *h = new QHBoxLayout(this);
    h->setObjectName(QStringLiteral("horizontalLayout"));
    h->setSpacing(0);
    h->setContentsMargins(9, 0, 9, 0);

    m_labelTitle = new QLabel(this);
    m_labelTitle->setObjectName(QStringLiteral("label_title"));
    h->addWidget(m_labelTitle);
    h->addStretch(1);   // реф. expanding QSpacerItem
    m_labelStatus = new QLabel(this);
    m_labelStatus->setObjectName(QStringLiteral("label_status"));
    h->addWidget(m_labelStatus);

    InitWidget();
}

void KOsdStatusLabel::InitWidget()
{
    // Реф.: label_title.setText(m_title); label_status.setText(m_value).
    m_labelTitle->setText(m_title);
    m_labelStatus->setText(m_value);
}

void KOsdStatusLabel::SetStringValue(const QString &v)
{
    // Реф.: обновляет ТОЛЬКО label_status (m_value-член не трогает).
    m_labelStatus->setText(v);
}

void KOsdStatusLabel::ConfirmKeyAct(int key)
{
    Q_UNUSED(key);
    // Реф.: m_msgProxy->SendToMainCtrl(m_msgType) (1-арг) — DEVICE-seam, в порте no-op.
}
