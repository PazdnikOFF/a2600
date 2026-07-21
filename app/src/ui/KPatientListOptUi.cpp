#include "KPatientListOptUi.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QKeyEvent>

KPatientListOptUi::KPatientListOptUi(QWidget *parent)
    : QWidget(parent)
{
    // Реф. ctor @0x7d2a78: setupUi → store view → InitWidget → InitWidgetFilter → InitConnect.
    setupUi();
}

void KPatientListOptUi::setupUi()
{
    setObjectName(QStringLiteral("KPatientListOptUi"));
    resize(270, 385);
    setMinimumSize(270, 385);
    setMaximumSize(270, 385);

    QVBoxLayout *v = new QVBoxLayout(this);
    v->setObjectName(QStringLiteral("verticalLayout"));
    v->setSpacing(13);
    v->setContentsMargins(30, 12, 19, -1);   // реф.: bottom = -1 (style default)

    auto mk = [&](const char *name, const QString &text) {
        QPushButton *b = new QPushButton(text, this);
        b->setObjectName(QString::fromLatin1(name));
        b->setMinimumSize(212, 46);
        b->setMaximumSize(212, 46);
        b->installEventFilter(this);
        v->addWidget(b);
        return b;
    };
    m_btnCheck = mk("btn_check", tr("TR_Exm") + QStringLiteral("(F1)"));
    m_btnAdd = mk("btn_add", tr("TR_Add") + QStringLiteral("(F2)"));
    m_btnEdit = mk("btn_edit", tr("TR_Edt") + QStringLiteral("(F6)"));
    m_btnDownload = mk("btn_download", tr("TR_Dwnld") + QStringLiteral("(F7)"));
    m_btnDelete = mk("btn_delete", tr("TR_Del") + QStringLiteral("(D)"));
    m_btnSet = mk("btn_set", tr("TR_Set") + QStringLiteral("(F8)"));

    v->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

    setTabOrder(m_btnCheck, m_btnAdd);
    setTabOrder(m_btnAdd, m_btnEdit);
    setTabOrder(m_btnEdit, m_btnDownload);
    setTabOrder(m_btnDownload, m_btnDelete);
    setTabOrder(m_btnDelete, m_btnSet);

    // Реф. InitConnect: clicked → слоты view. В порте — свои сигналы (VIEW-seam).
    connect(m_btnCheck, &QPushButton::clicked, this, &KPatientListOptUi::sigExam);
    connect(m_btnAdd, &QPushButton::clicked, this, &KPatientListOptUi::sigAdd);
    connect(m_btnEdit, &QPushButton::clicked, this, &KPatientListOptUi::sigEdit);
    connect(m_btnDownload, &QPushButton::clicked, this, &KPatientListOptUi::sigDownload);
    connect(m_btnDelete, &QPushButton::clicked, this, &KPatientListOptUi::sigDelete);
    connect(m_btnSet, &QPushButton::clicked, this, &KPatientListOptUi::sigSetup);
}

void KPatientListOptUi::MoveFocusToFirstWidget()
{
    if (m_btnCheck)
        m_btnCheck->setFocus();
}

bool KPatientListOptUi::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::KeyPress && static_cast<QKeyEvent *>(e)->key() == Qt::Key_Tab) {
        if (o == m_btnSet) {
            emit SigToFocusOutCurrentOpt();
            return true;
        }
    }
    return QWidget::eventFilter(o, e);
}
