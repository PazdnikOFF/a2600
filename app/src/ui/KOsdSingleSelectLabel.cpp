#include "KOsdSingleSelectLabel.h"
#include "Theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>

KOsdSingleSelectLabel::KOsdSingleSelectLabel(const KOsdSingleSelectLabelConfig &cfg,
                                             QWidget *parent, ConfirmCallback action)
    : KFrame(parent)
    , m_cfg(cfg)
    , m_action(std::move(action))
{
    // Реф. ctor @0x582c30 + InitWidget @0x582be0: QHBoxLayout(9,0,0,0) + label_icon + текст,
    // фикс 250. Начальное состояние — UnChecked (normal-иконка).
    setObjectName(QStringLiteral("KOsdSingleSelectLabel"));
    setFixedSize(250, 44);   // реф. min=max 0xFA=250 (ширина); высота — под строку списка

    QHBoxLayout *h = new QHBoxLayout(this);
    h->setObjectName(QStringLiteral("horizontalLayout"));
    h->setContentsMargins(9, 0, 0, 0);

    m_labelIcon = new QLabel(this);
    m_labelIcon->setObjectName(QStringLiteral("label_icon"));
    m_labelIcon->setFixedSize(24, 24);
    m_labelIcon->setScaledContents(true);
    h->addWidget(m_labelIcon);

    m_labelText = new QLabel(this);
    m_labelText->setText(m_cfg.text);   // реф. InitWidget: textLabel.setText(config.text)
    h->addWidget(m_labelText, 1);

    UnChecked();
}

QPixmap KOsdSingleSelectLabel::osdIcon(const QString &name) const
{
    // DEVICE-STUB KDisplayOption::GetOsdIconPixmap → грузим из темы (как KOsdMenuCell).
    return QPixmap(theme::asset(QStringLiteral("black/osd/") + name));
}

void KOsdSingleSelectLabel::Checked()
{
    // Реф. @0x583218.
    m_labelIcon->setPixmap(osdIcon(QStringLiteral("singlechoice_select.png")));
}

void KOsdSingleSelectLabel::UnChecked()
{
    // Реф. @0x5832f0.
    m_labelIcon->setPixmap(osdIcon(QStringLiteral("singlechoice_normal.png")));
}

void KOsdSingleSelectLabel::ConfirmKeyAct(int value)
{
    // Реф. @0x582bf0: action->call(*this, value). У нас — инъектируемый колбэк.
    // Реф.-стратегия: KUiMsgProxy::SendToMainCtrl(cfg.msgType, cfg.msgParam, value) — DEVICE-seam.
    if (m_action)
        m_action(m_cfg, value);
}
