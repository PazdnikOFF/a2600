#include "KExamListOptUi.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QKeyEvent>

KExamListOptUi::KExamListOptUi(QWidget *parent)
    : QWidget(parent)
{
    // Реф. ctor @0x7f1630: setupUi → InitWidget (тексты+суффиксы) → InitWidgetFilter →
    // InitConnect (clicked→слоты view; в порте — свои сигналы).
    setupUi();
}

void KExamListOptUi::setupUi()
{
    setObjectName(QStringLiteral("KExamListOptUi"));
    resize(270, 385);
    setMinimumSize(270, 385);
    setMaximumSize(270, 385);

    QVBoxLayout *v = new QVBoxLayout(this);
    v->setObjectName(QStringLiteral("verticalLayout"));
    v->setSpacing(13);
    v->setContentsMargins(30, 12, 19, 9);

    // Тексты — реф. tr(base)+hotkey-суффикс (tr-ключи видны в превью без перевода).
    auto mk = [&](const char *name, const QString &text) {
        QPushButton *b = new QPushButton(text, this);
        b->setObjectName(QString::fromLatin1(name));
        b->setMinimumSize(212, 46);
        b->setMaximumSize(212, 46);
        b->installEventFilter(this);   // реф. InitWidgetFilter: Tab-циклинг
        v->addWidget(b);
        return b;
    };
    m_btnReport = mk("btn_report", tr("TR_Rprt") + QStringLiteral("(F1)"));
    m_btnExport = mk("btn_export", tr("TR_Expt") + QStringLiteral("(F2)"));
    m_btnUpload = mk("btn_upload", tr("TR_DUpload") + QStringLiteral("(F6)"));
    m_btnCancleCheck = mk("btn_cancle_check", tr("TR_CExam") + QStringLiteral("(F7)"));
    m_btnDelete = mk("btn_delete", tr("TR_Del") + QStringLiteral("(D)"));
    m_btnSet = mk("btn_set", tr("TR_Set") + QStringLiteral("(F8)"));

    v->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

    // Tab-порядок (реф. setTabOrder цепочкой).
    setTabOrder(m_btnReport, m_btnExport);
    setTabOrder(m_btnExport, m_btnUpload);
    setTabOrder(m_btnUpload, m_btnCancleCheck);
    setTabOrder(m_btnCancleCheck, m_btnDelete);
    setTabOrder(m_btnDelete, m_btnSet);

    // Реф. InitConnect: clicked→слоты view. В порте — свои сигналы (VIEW-seam).
    connect(m_btnReport, &QPushButton::clicked, this, &KExamListOptUi::sigReport);
    connect(m_btnExport, &QPushButton::clicked, this, &KExamListOptUi::sigExport);
    connect(m_btnUpload, &QPushButton::clicked, this, &KExamListOptUi::sigUpload);
    connect(m_btnCancleCheck, &QPushButton::clicked, this, &KExamListOptUi::sigCancelExam);
    connect(m_btnDelete, &QPushButton::clicked, this, &KExamListOptUi::sigDelete);
    connect(m_btnSet, &QPushButton::clicked, this, &KExamListOptUi::sigSetup);
}

void KExamListOptUi::MoveFocusToFirstWidget()
{
    if (m_btnReport)
        m_btnReport->setFocus();
}

bool KExamListOptUi::eventFilter(QObject *o, QEvent *e)
{
    // Реф. eventFilter: Tab с последней кнопки уводит фокус с панели.
    if (e->type() == QEvent::KeyPress && static_cast<QKeyEvent *>(e)->key() == Qt::Key_Tab) {
        if (o == m_btnSet) {   // конец цикла
            emit SigToFocusOutCurrentOpt();
            return true;
        }
    }
    return QWidget::eventFilter(o, e);
}
