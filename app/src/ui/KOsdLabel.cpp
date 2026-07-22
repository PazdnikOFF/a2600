#include "KOsdLabel.h"

#include <QHBoxLayout>
#include <QLabel>

KOsdLabel::KOsdLabel(const KOsdLabelConfig &cfg, QWidget *parent)
    : KFrame(parent)
    , m_text(cfg.text)
    , m_action(cfg.action)
{
    // Реф. ctor @0x480a50 + setupUi: QHBoxLayout(0, margins 9,0,0,0) + label_title, фикс 250×32.
    setObjectName(QStringLiteral("KOsdLabel"));
    setFixedSize(250, 32);   // реф. min=max (0xfa, 0x20)

    QHBoxLayout *h = new QHBoxLayout(this);
    h->setObjectName(QStringLiteral("horizontalLayout"));
    h->setSpacing(0);
    h->setContentsMargins(9, 0, 0, 0);

    m_labelTitle = new QLabel(this);
    m_labelTitle->setObjectName(QStringLiteral("label_title"));
    m_labelTitle->setText(m_text);   // реф. InitWidget: label_title.setText(config.text)
    h->addWidget(m_labelTitle);
}

void KOsdLabel::ConfirmKeyAct(int key)
{
    // Реф. @0x480...: fn = m_action; if (fn) fn(GetStartPoint(), key, GetLocatedMenu()).
    // action = Xxx::EnterMenu — открытие под-подменю (DEVICE-seam, в порте инъект/no-op).
    if (m_action)
        m_action(GetStartPoint(), key, GetLocatedMenu());
}
