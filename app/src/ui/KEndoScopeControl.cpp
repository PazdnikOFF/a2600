#include "KEndoScopeControl.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

namespace {
QString U(const char *s) { return QString::fromUtf8(s); }
QPushButton *ctlBtn(QWidget *p, const char *name, const QString &text, int minW, int maxW)
{
    QPushButton *b = new QPushButton(text, p);
    b->setObjectName(QString::fromLatin1(name));
    b->setMinimumWidth(minW); b->setMaximumWidth(maxW);
    return b;
}
QLabel *devLbl(QWidget *p, const char *name, const QString &placeholder)
{
    QLabel *l = new QLabel(placeholder, p);   // значение — device (плейсхолдер-текст)
    l->setObjectName(QString::fromLatin1(name));
    l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return l;
}
} // namespace

KEndoScopeControl::KEndoScopeControl(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x740130: KDialog(modal=false) → setupUi → SetKStyle(2) → title TR_EControl →
    // KControlProc device-state → InitUseTimesCtrl/InitMatchProCtrl → AllConnect.
    setupUi();
    SetKStyle(KDLG_W460);              // реф. SetKStyle(2)
    SetTitle(tr("TR_EControl"));
}

void KEndoScopeControl::setupUi()
{
    setObjectName(QStringLiteral("KEndoScopeControl"));
    resize(640, 480);

    QWidget *host = ContentArea();
    QVBoxLayout *root = new QVBoxLayout(host);
    root->setSpacing(30);
    root->setContentsMargins(20, 60, 20, 40);

    // === Группа контроля числа использований ===
    QGroupBox *grpUse = new QGroupBox(tr("TR_UOEControl"), host);
    grpUse->setObjectName(QStringLiteral("groupBox"));
    QGridLayout *g = new QGridLayout(grpUse);
    g->setContentsMargins(20, 20, 9, 9); g->setSpacing(20);
    QLabel *lEDate = new QLabel(tr("TR_EDate:"), grpUse); lEDate->setObjectName(QStringLiteral("label_expiryDate_tip"));
    g->addWidget(lEDate, 0, 0);
    g->addWidget(devLbl(grpUse, "label_expiryDate", U("截止日期")), 0, 1);
    QLabel *lRem = new QLabel(tr("TR_Rmnng:"), grpUse); lRem->setObjectName(QStringLiteral("label_remainDays_tips"));
    g->addWidget(lRem, 1, 0);
    g->addWidget(devLbl(grpUse, "label_remainDays", U("剩余天数")), 1, 1);
    QLabel *lUses = new QLabel(tr("TR_NORUses:"), grpUse); lUses->setObjectName(QStringLiteral("label"));
    lUses->setWordWrap(true);
    g->addWidget(lUses, 2, 0);
    g->addWidget(devLbl(grpUse, "label_remainTime", U("次数")), 2, 1);
    QHBoxLayout *h2 = new QHBoxLayout();
    h2->addStretch(1);
    h2->addWidget(ctlBtn(grpUse, "btn_changeTimesCtl", U("开启/解除管控"), 120, 160));
    h2->addStretch(1);
    h2->addWidget(ctlBtn(grpUse, "btn_importDelayLic", tr("TR_IAuthorization"), 120, 160));
    h2->addStretch(1);
    g->addLayout(h2, 3, 0, 1, 2);
    root->addWidget(grpUse);

    // === Группа контроля совместимых процессоров ===
    QGroupBox *grpPro = new QGroupBox(tr("TR_CPControl"), host);
    grpPro->setObjectName(QStringLiteral("groupBox_2"));
    QGridLayout *g2 = new QGridLayout(grpPro);
    g2->setContentsMargins(20, 20, 9, 9); g2->setSpacing(20);
    QLabel *lAmt = new QLabel(tr("TR_CPAmount:"), grpPro); lAmt->setObjectName(QStringLiteral("label_3"));
    g2->addWidget(lAmt, 0, 0);
    g2->addWidget(devLbl(grpPro, "label_matchProNum", QStringLiteral("Num")), 0, 1);
    g2->addWidget(ctlBtn(grpPro, "btn_viewMatchPro", tr("TR_Vw"), 120, 120), 0, 2);
    QHBoxLayout *h3 = new QHBoxLayout();
    h3->addStretch(1);
    h3->addWidget(ctlBtn(grpPro, "btn_changeMatchCtl", U("开启/解除管控"), 120, 160));
    h3->addStretch(1);
    h3->addWidget(ctlBtn(grpPro, "btn_importProcessorLic", tr("TR_IAuthorization"), 120, 160));
    h3->addStretch(1);
    g2->addLayout(h3, 1, 0, 1, 3);
    root->addWidget(grpPro);

    root->addStretch(1);

    // === Exit ===
    QHBoxLayout *h = new QHBoxLayout();
    h->addStretch(1);
    QPushButton *btnExit = ctlBtn(host, "btn_exit", tr("TR_Ext"), 120, 120);
    h->addWidget(btnExit);
    h->addStretch(1);
    root->addLayout(h);

    connect(btnExit, &QPushButton::clicked, this, &QWidget::close);   // реф. btn_exit→close
    // Прочие кнопки → ChangeUseTimesCtrlState/ImportDelayLic/ChangeMatchProCtrlState/
    // ViewMatchProList/ImportMatchProLic — DEVICE, не подключаем.
}
