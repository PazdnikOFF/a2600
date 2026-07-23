#include "KUserSrvSet.h"

#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

KUserSrvSet::KUserSrvSet(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x70c0b8: KDialog(modal=false) → setupUi → SetKStyle(2) → title TR_Svce →
    // InitWidget (info-метки + роль-гейтинг, device) → QTimer(recovery) → AllConnect.
    setupUi();
    SetKStyle(KDLG_W460);              // реф. SetKStyle(2)
    SetTitle(tr("TR_Svce"));
}

void KUserSrvSet::setupUi()
{
    setObjectName(QStringLiteral("KUserSrvSet"));
    resize(460, 768);

    QWidget *host = ContentArea();
    QVBoxLayout *root = new QVBoxLayout(host);
    root->setContentsMargins(20, 40, 20, 9);

    auto mkBtn = [&](QWidget *p, const char *name, const QString &text, int minW) {
        QPushButton *b = new QPushButton(text, p);
        b->setObjectName(QString::fromLatin1(name));
        b->setMinimumWidth(minW);
        return b;
    };

    // ===================== groupBoxServer (функции) =====================
    QGroupBox *grpSrv = new QGroupBox(tr("TR_Fnctn"), host);
    grpSrv->setObjectName(QStringLiteral("groupBoxServer"));
    QGridLayout *g6 = new QGridLayout(grpSrv);
    // Секция Log.
    QLabel *lblLog = new QLabel(tr("TR_Log:"), grpSrv); lblLog->setObjectName(QStringLiteral("label_7"));
    g6->addWidget(lblLog, 0, 0);
    QHBoxLayout *hLog = new QHBoxLayout();
    hLog->addWidget(mkBtn(grpSrv, "btn_Backup", tr("TR_Bkp"), 100));
    hLog->addWidget(mkBtn(grpSrv, "btn_View", tr("TR_Vw"), 100));
    g6->addLayout(hLog, 1, 1);
    // Секция MMControl.
    QLabel *lblMM = new QLabel(tr("TR_MMControl:"), grpSrv); lblMM->setObjectName(QStringLiteral("label_6"));
    g6->addWidget(lblMM, 2, 0, 1, 2);
    QHBoxLayout *hMM = new QHBoxLayout();
    hMM->addWidget(mkBtn(grpSrv, "btn_ProcessorControl", tr("TR_Prcssr"), 100));
    hMM->addWidget(mkBtn(grpSrv, "btn_EndoControl", tr("TR_Escpe"), 100));
    g6->addLayout(hMM, 3, 1);
    // Секция Otr.
    QLabel *lblOtr = new QLabel(tr("TR_Otr:"), grpSrv); lblOtr->setObjectName(QStringLiteral("label_10"));
    g6->addWidget(lblOtr, 4, 0);
    QHBoxLayout *hUp = new QHBoxLayout();
    hUp->addWidget(mkBtn(grpSrv, "btn_Upgrade", tr("TR_Ugde"), 120));
    hUp->addWidget(mkBtn(grpSrv, "btn_Recovery", tr("TR_Rcvry"), 120));
    g6->addLayout(hUp, 5, 1);
    QHBoxLayout *hCal = new QHBoxLayout();
    hCal->addWidget(mkBtn(grpSrv, "btn_videoCal", tr("TR_VCal"), 120));
    hCal->addWidget(mkBtn(grpSrv, "btn_lightConfig", tr("TR_LSConfiguration"), 120));  // реф. роль-гейт role<3 (device)
    g6->addLayout(hCal, 6, 1);
    root->addWidget(grpSrv);

    // ===================== groupBoxInfo (инфо устройства) =====================
    QGroupBox *grpInfo = new QGroupBox(tr("TR_EInformaion"), host);
    grpInfo->setObjectName(QStringLiteral("groupBoxInfo"));
    QGridLayout *gInfo = new QGridLayout(grpInfo);
    int r = 0;
    auto infoRow = [&](const QString &cap, const char *capName, const char *valName) {
        QHBoxLayout *h = new QHBoxLayout();
        QLabel *c = new QLabel(cap, grpInfo); c->setObjectName(QString::fromLatin1(capName));
        h->addWidget(c);
        QLabel *v = new QLabel(grpInfo); v->setObjectName(QString::fromLatin1(valName));   // device
        h->addWidget(v, 1);
        gInfo->addLayout(h, r++, 0);
    };
    infoRow(tr("TR_IPSN:"), "label_processorSNTips", "label_processorSN");
    infoRow(tr("TR_AWTOCMLamp:"), "label_currentLampWorkTimeTips", "label_currentLampWorkTime");
    infoRow(tr("TR_AWTOAMLamp:"), "label_allLampWorkTimeTips", "label_allLampWorkTime");
    infoRow(tr("TR_ESN:"), "label_endoSNTips", "label_endoSN");
    infoRow(tr("TR_FOUse:"), "label_endoUseCountTips", "label_endoUseCount");
    root->addWidget(grpInfo);

    root->addStretch(1);

    // ===================== Exit (центрирован) =====================
    QFrame *frameBtn = new QFrame(host);
    frameBtn->setObjectName(QStringLiteral("frameButton"));
    QHBoxLayout *h2 = new QHBoxLayout(frameBtn);
    h2->addStretch(1);
    QPushButton *btnExit = new QPushButton(tr("TR_Ext"), frameBtn);
    btnExit->setObjectName(QStringLiteral("btn_exit"));
    btnExit->setFixedWidth(120);   // реф. min=max 120
    h2->addWidget(btnExit);
    h2->addStretch(1);
    root->addWidget(frameBtn);

    connect(btnExit, &QPushButton::clicked, this, &QWidget::close);   // реф. ClickExit→close
    // Прочие кнопки → под-диалоги/device-операции — DEVICE, не подключаем.
}

void KUserSrvSet::InterfaceJump(int moduleId)
{
    // Реф.: запомнить модуль перехода и закрыть диалог — открывает следующий экран
    // уже вызывающий (OpenUserSrvSetDlg), а не сам диалог.
    m_jumpModule = moduleId;
    close();
}
