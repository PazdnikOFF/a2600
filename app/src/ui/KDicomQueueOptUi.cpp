#include "KDicomQueueOptUi.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QKeyEvent>

KDicomQueueOptUi::KDicomQueueOptUi(QWidget *parent)
    : QWidget(parent)
{
    // Реф. ctor @0x806890: setupUi (инлайн) + InitConnect + InitWidgetFilter.
    setupUi();
}

void KDicomQueueOptUi::setupUi()
{
    setObjectName(QStringLiteral("KDicomQueueOptUi"));
    resize(270, 385);
    setMinimumSize(270, 385);
    setMaximumSize(270, 385);

    QVBoxLayout *v = new QVBoxLayout(this);
    v->setObjectName(QStringLiteral("verticalLayout"));
    v->setSpacing(13);
    v->setContentsMargins(30, 12, 19, -1);

    auto mk = [&](const char *name, const QString &text) {
        QPushButton *b = new QPushButton(text, this);
        b->setObjectName(QString::fromLatin1(name));
        b->setMinimumSize(212, 46);
        b->setMaximumSize(212, 46);
        b->installEventFilter(this);
        v->addWidget(b);
        return b;
    };
    m_btnRefresh = mk("btn_refresh", tr("TR_Rfsh") + QStringLiteral("(F1)"));
    m_btnResend = mk("btn_resend", tr("TR_Rsnd") + QStringLiteral("(F2)"));
    m_btnDelete = mk("btn_delete", tr("TR_Del") + QStringLiteral("(D)"));

    v->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

    setTabOrder(m_btnRefresh, m_btnResend);
    setTabOrder(m_btnResend, m_btnDelete);

    // Реф. InitConnect: clicked → слоты view. В порте — свои сигналы.
    connect(m_btnRefresh, &QPushButton::clicked, this, &KDicomQueueOptUi::sigRefresh);
    connect(m_btnResend, &QPushButton::clicked, this, &KDicomQueueOptUi::sigResend);
    connect(m_btnDelete, &QPushButton::clicked, this, &KDicomQueueOptUi::sigDelete);
}

void KDicomQueueOptUi::MoveFocusToFirstWidget()
{
    if (m_btnRefresh)
        m_btnRefresh->setFocus(Qt::OtherFocusReason);
}

bool KDicomQueueOptUi::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::KeyPress && static_cast<QKeyEvent *>(e)->key() == Qt::Key_Tab) {
        if (o == m_btnDelete) {   // Tab с последней кнопки
            emit SigToFocusOutCurrentOpt();
            return true;
        }
    }
    return QWidget::eventFilter(o, e);
}
