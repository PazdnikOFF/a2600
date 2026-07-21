#include "KMsgPopup.h"

#include <QTimer>

KMsgPopup::KMsgPopup(QWidget *parent)
    : QLabel(parent)
{
    // Реф. ctor @0x684788: m_rect от device-конфига (стаб) → m_pLabel → UpdatePostion →
    // wordwrap/center → hidden → таймер → connect timeout→HideLabelInfo → status=1.
    m_rect = QRect(0, 0, 300, 60);   // DEVICE-STUB (реф. GetSoftEndoViewConf)
    m_pLabel = new QLabel(this);
    UpdatePostion(QRect());
    m_pLabel->setWordWrap(true);
    m_pLabel->setAlignment(Qt::AlignCenter);
    setVisible(false);
    m_pTimer = new QTimer(this);
    m_pTimer->setInterval(5000);   // реф. 5000 мс автоскрытие
    connect(m_pTimer, &QTimer::timeout, this, &KMsgPopup::HideLabelInfo);
    m_status = 1;
}

void KMsgPopup::UpdatePostion(const QRect &rect)
{
    // Реф. @0x684638: null → device-позиция над видео (стаб — используем m_rect); иначе rect.
    const QRect r = rect.isNull() ? m_rect : rect;
    setGeometry(r);
    if (m_pLabel)
        m_pLabel->setGeometry(0, 0, r.width() - 12, r.height());
}

void KMsgPopup::Display(const QString &msg, bool flag)
{
    // Реф. @0x684a08.
    if (msg.isEmpty())
        return;
    m_flag = flag;
    if (m_status == 1) {
        m_pTimer->start();
        m_pLabel->setText(msg);
        setVisible(true);
        m_status = 2;
    } else {
        // Уже показывается → очередь (не затирать текущий тост).
        m_pending = msg;
    }
}

void KMsgPopup::HideLabelInfo()
{
    // Реф. @0x684e78: стоп таймера, status=1, скрыть или показать отложенное.
    m_pTimer->stop();
    m_status = 1;
    if (m_pending.isEmpty()) {
        setVisible(false);
    } else {
        const QString next = m_pending;
        m_pending.clear();
        Display(next, m_flag);
    }
}
