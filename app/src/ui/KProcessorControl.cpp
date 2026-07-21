#include "KProcessorControl.h"

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
} // namespace

KProcessorControl::KProcessorControl(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x73b870: KDialog(modal=false) → setupUi → SetKStyle(2) → title TR_PControl →
    // KControlINI::ReadMcTime/ReadMcEndo (device) → InitWidget/InitTimeControl/
    // InitEndoMatchControl → AllConnect.
    setupUi();
    SetKStyle(KDLG_W460);              // реф. SetKStyle(2)
    SetTitle(tr("TR_PControl"));
}

void KProcessorControl::setupUi()
{
    setObjectName(QStringLiteral("KProcessorControl"));
    resize(640, 480);

    QWidget *host = ContentArea();
    QVBoxLayout *root = new QVBoxLayout(host);
    root->setSpacing(30);
    root->setContentsMargins(20, 60, 20, 60);

    // === Группа контроля времени ===
    QGroupBox *grpTime = new QGroupBox(tr("TR_TControl"), host);
    grpTime->setObjectName(QStringLiteral("groupBox"));
    QGridLayout *g = new QGridLayout(grpTime);
    g->setContentsMargins(20, 20, 20, 20); g->setSpacing(20);
    QLabel *lEDate = new QLabel(tr("TR_EDate:"), grpTime); lEDate->setObjectName(QStringLiteral("label_expiryDate"));
    g->addWidget(lEDate, 0, 0);
    QLabel *lEDateV = new QLabel(U("到期时间"), grpTime); lEDateV->setObjectName(QStringLiteral("label_expiryDate_2"));
    g->addWidget(lEDateV, 0, 1);   // значение — device
    QLabel *lRem = new QLabel(tr("TR_Rmnng:"), grpTime); lRem->setObjectName(QStringLiteral("label_remaining"));
    g->addWidget(lRem, 1, 0);
    QLabel *lRemV = new QLabel(U("剩余天数"), grpTime); lRemV->setObjectName(QStringLiteral("label_remainingDays"));
    g->addWidget(lRemV, 1, 1);   // значение — device
    QHBoxLayout *hTime = new QHBoxLayout();
    hTime->addStretch(1);
    hTime->addWidget(ctlBtn(grpTime, "btn_timeControl", U("开启/解除管控"), 120, 160));
    hTime->addStretch(1);
    hTime->addWidget(ctlBtn(grpTime, "btn_importDelayLicense", tr("TR_IAuthorization"), 120, 160));
    hTime->addStretch(1);
    g->addLayout(hTime, 2, 0, 1, 2);
    root->addWidget(grpTime);

    // === Группа контроля совпадения эндоскопов ===
    QGroupBox *grpEndo = new QGroupBox(tr("TR_CEControl"), host);
    grpEndo->setObjectName(QStringLiteral("groupBox_matchEndo"));
    QGridLayout *g2 = new QGridLayout(grpEndo);
    g2->setContentsMargins(20, 20, 20, 20); g2->setSpacing(20);
    QLabel *lNum = new QLabel(tr("TR_TNOEndoscope:"), grpEndo);
    lNum->setObjectName(QStringLiteral("label")); lNum->setWordWrap(true);
    g2->addWidget(lNum, 0, 0);
    QLabel *lNumV = new QLabel(U("镜体数量"), grpEndo); lNumV->setObjectName(QStringLiteral("label_endoScopeNum"));
    g2->addWidget(lNumV, 0, 2);   // значение — device
    g2->addWidget(ctlBtn(grpEndo, "btn_view", tr("TR_Vw"), 120, 120), 0, 4);
    QHBoxLayout *hEndo = new QHBoxLayout();
    hEndo->addStretch(1);
    hEndo->addWidget(ctlBtn(grpEndo, "btn_endoControl", U("开启/解除管控"), 120, 160));
    hEndo->addStretch(1);
    hEndo->addWidget(ctlBtn(grpEndo, "btn_importEndoLicense", tr("TR_IAuthorization"), 120, 160));
    hEndo->addStretch(1);
    g2->addLayout(hEndo, 1, 0, 1, 5);
    root->addWidget(grpEndo);

    root->addStretch(1);

    // === Exit ===
    QHBoxLayout *h3 = new QHBoxLayout();
    h3->addStretch(1);
    QPushButton *btnExit = ctlBtn(host, "btn_exit", tr("TR_Ext"), 120, 120);
    h3->addWidget(btnExit);
    h3->addStretch(1);
    root->addLayout(h3);

    connect(btnExit, &QPushButton::clicked, this, &QWidget::close);   // реф. btn_exit→close
    // Прочие кнопки → ChangeTimeCtlState/ImportDelayLicense/ChangeEndoMatchCtlState/
    // ImportMatchEndoLicense/ViewEndoScopeList — DEVICE, не подключаем.
}
