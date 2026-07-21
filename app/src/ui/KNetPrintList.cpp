#include "KNetPrintList.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>

KNetPrintList::KNetPrintList(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x78cc90: KDialog(modal=false) + KObject → setupUi → SetTitle(TR_Sch) →
    // setStyleSheet(тёмный) → connects → QTimer.start → кнопки disabled → SubscribeMsg(0x2b2b).
    setupUi();
    SetTitle(tr("TR_Sch"));
}

void KNetPrintList::setupUi()
{
    setObjectName(QStringLiteral("KNetPrintList"));
    resize(646, 490);

    QWidget *host = ContentArea();
    host->setStyleSheet(QStringLiteral(
        "QWidget{background-color: rgba(26,26,26,255);}"));   // реф. тёмный фон
    QGridLayout *g = new QGridLayout(host);
    g->setObjectName(QStringLiteral("gridLayout"));
    g->setContentsMargins(20, 30, 20, 20);

    // Фрейм со списком принтеров.
    QFrame *frame = new QFrame(host);
    frame->setObjectName(QStringLiteral("frame"));
    QGridLayout *g2 = new QGridLayout(frame);
    g2->setObjectName(QStringLiteral("gridLayout_2"));
    QLabel *lblMsg = new QLabel(tr("TR_Schng."), frame);   // реф. «поиск…»
    lblMsg->setObjectName(QStringLiteral("label_msg"));
    g2->addWidget(lblMsg, 0, 0);
    QListWidget *list = new QListWidget(frame);            // реф. device-скан (KHalPrinterAPI)
    list->setObjectName(QStringLiteral("m_pfindIpList"));
    list->setFocusPolicy(Qt::ClickFocus);
    list->setTabKeyNavigation(false);
    g2->addWidget(list, 1, 0);
    g->addWidget(frame, 0, 0);

    // Ряд кнопок: [stretch] Search [gap] OK [gap] Cancel [stretch]; все стартуют disabled.
    QHBoxLayout *h = new QHBoxLayout();
    h->setObjectName(QStringLiteral("horizontalLayout"));
    auto mkBtn = [&](const char *name, const QString &text) {
        QPushButton *b = new QPushButton(text, host);
        b->setObjectName(QString::fromLatin1(name));
        b->setEnabled(false);   // реф. setEnabled(false) на старте
        return b;
    };
    QPushButton *btnSearch = mkBtn("m_pSearchBtn", tr("TR_Sch"));   // реф. time2RefreshPrinter (device)
    QPushButton *btnOk = mkBtn("m_pOkBtn", tr("TR_OK"));            // реф. ClickBtnOK (device)
    QPushButton *btnCancel = mkBtn("m_pCancelBtn", tr("TR_Ccl"));
    h->addStretch(1);
    h->addWidget(btnSearch);
    h->addSpacing(30);
    h->addWidget(btnOk);
    h->addSpacing(30);
    h->addWidget(btnCancel);
    h->addStretch(1);
    g->addLayout(h, 1, 0);

    connect(btnCancel, &QPushButton::clicked, this, &QWidget::close);   // реф. Cancel→close
}
