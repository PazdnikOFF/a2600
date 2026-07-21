#include "KDICOMServiceEditDlg.h"

#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegExpValidator>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

KDICOMServiceEditDlg::KDICOMServiceEditDlg(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x5b1a18: KDialog(modal=false) → setupUi → SetKStyle(6) → title по режиму →
    // ping/echo QTimer → InitWidget → LoadData(device) → InitConnect.
    setupUi();
    SetKStyle(KDLG_W700);              // реф. SetKStyle(6)
    SetTitle(tr("TR_ADService"));      // реф. add-режим (cur==null); edit → TR_EDService
}

void KDICOMServiceEditDlg::setupUi()
{
    setObjectName(QStringLiteral("KDICOMServiceEditDlg"));
    resize(639, 568);

    QWidget *host = ContentArea();
    QVBoxLayout *root = new QVBoxLayout(host);
    root->setObjectName(QStringLiteral("verticalLayout"));
    root->setContentsMargins(20, 40, 20, 20);   // реф. отступы (20,40,20,20)

    QGridLayout *g = new QGridLayout();
    g->setObjectName(QStringLiteral("gridLayout"));

    // (0) Тип сервиса — статичный список.
    g->addWidget(new QLabel(tr("TR_SType"), host), 0, 0);
    QComboBox *cmbType = new QComboBox(host);
    cmbType->setObjectName(QStringLiteral("cmb_serviceType"));
    cmbType->addItem(tr("TR_Strge"), 0);
    cmbType->addItem(tr("TR_SCommitment"), 1);
    cmbType->addItem(tr("TR_Wrklst"), 2);
    cmbType->addItem(tr("TR_MPPS"), 3);
    g->addWidget(cmbType, 0, 1);

    // (1) Имя сервиса — maxLen 16.
    QLabel *lbName = new QLabel(tr("TR_SName:"), host); lbName->setObjectName(QStringLiteral("label_2"));
    g->addWidget(lbName, 1, 0);
    QLineEdit *edName = new QLineEdit(host);
    edName->setObjectName(QStringLiteral("ledt_serviceName"));
    edName->setMaxLength(16);
    g->addWidget(edName, 1, 1);

    // (2) Режим адреса (IP/домен) + контейнер + Ping + результат.
    QComboBox *cmbServer = new QComboBox(host);
    cmbServer->setObjectName(QStringLiteral("cmb_server"));
    cmbServer->setFocusPolicy(Qt::NoFocus);       // реф. NoFocus
    cmbServer->addItem(tr("TR_SIP:"));
    cmbServer->addItem(tr("TR_SDName"));
    g->addWidget(cmbServer, 2, 0);
    // Реф. widget_server: IPAddr (KIpAddrEdit) и ledit_domain перекрываются, видимость по
    // cmb_server. Моделируем QStackedWidget.
    QStackedWidget *stkServer = new QStackedWidget(host);
    stkServer->setObjectName(QStringLiteral("widget_server"));
    stkServer->setMinimumSize(192, 32);
    QLineEdit *edIP = new QLineEdit(stkServer);     // реф. KIpAddrEdit → QLineEdit c маской
    edIP->setObjectName(QStringLiteral("IPAddr"));
    edIP->setInputMask(QStringLiteral("000.000.000.000;_"));
    QLineEdit *edDomain = new QLineEdit(stkServer);
    edDomain->setObjectName(QStringLiteral("ledit_domain"));
    stkServer->addWidget(edIP);
    stkServer->addWidget(edDomain);
    g->addWidget(stkServer, 2, 1);
    connect(cmbServer, QOverload<int>::of(&QComboBox::currentIndexChanged),
            stkServer, &QStackedWidget::setCurrentIndex);   // реф. SlotToChangeInputServerType
    QPushButton *btnPing = new QPushButton(QStringLiteral("Ping"), host);   // реф. литерал, не TR
    btnPing->setObjectName(QStringLiteral("btn_Ping"));
    btnPing->setFixedSize(120, 30);
    g->addWidget(btnPing, 2, 2);
    QLabel *lbPingRes = new QLabel(host);
    lbPingRes->setObjectName(QStringLiteral("label_PingRes"));
    lbPingRes->setMinimumWidth(80);
    lbPingRes->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    g->addWidget(lbPingRes, 2, 3);
    // btn_Ping → Ping (ICMP) — DEVICE, не подключаем.

    // (3) Порт + Echo(верификация) + результат.
    QLabel *lbPort = new QLabel(tr("TR_Prt:"), host); lbPort->setObjectName(QStringLiteral("label_4"));
    g->addWidget(lbPort, 3, 0);
    QLineEdit *edPort = new QLineEdit(host);
    edPort->setObjectName(QStringLiteral("ledt_servicePort"));
    edPort->setValidator(new QRegExpValidator(
        QRegExp(QStringLiteral("^([0-9]|[1-9][0-9]{1,3}|[1-5][0-9]{4}|6[0-4][0-9]{3}"
                               "|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])$")), edPort));  // 0..65535
    g->addWidget(edPort, 3, 1);
    QPushButton *btnEcho = new QPushButton(tr("TR_Vrfctn"), host);
    btnEcho->setObjectName(QStringLiteral("btn_Echo"));
    btnEcho->setFixedSize(120, 30);
    g->addWidget(btnEcho, 3, 2);
    QLabel *lbEchoRes = new QLabel(host);
    lbEchoRes->setObjectName(QStringLiteral("label_EchoRes"));
    lbEchoRes->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    g->addWidget(lbEchoRes, 3, 3);
    // btn_Echo → EchoService (C-ECHO) — DEVICE, не подключаем.

    // (4) AE Title — maxLen 16.
    QLabel *lbAE = new QLabel(tr("TR_ATitle:"), host); lbAE->setObjectName(QStringLiteral("label_5"));
    g->addWidget(lbAE, 4, 0);
    QLineEdit *edAE = new QLineEdit(host);
    edAE->setObjectName(QStringLiteral("ledt_serviceAE"));
    edAE->setMaxLength(16);
    g->addWidget(edAE, 4, 1);

    // (5) Тип storage-commitment — device-populated (пусто).
    QLabel *lbCmt = new QLabel(tr("TR_DSServer"), host);
    lbCmt->setObjectName(QStringLiteral("label_cmtType"));
    lbCmt->setMinimumWidth(100); lbCmt->setWordWrap(true);
    g->addWidget(lbCmt, 5, 0);
    QComboBox *cmbCmt = new QComboBox(host);
    cmbCmt->setObjectName(QStringLiteral("cmb_cmtType"));
    cmbCmt->addItem(QString());   // реф. пустой первый элемент + список из KDICOMLocalConf (device)
    g->addWidget(cmbCmt, 5, 1);

    // (6) Макс. результатов — maxLen 3, дефолт "99", validator 0..999.
    QLabel *lbMax = new QLabel(tr("TR_MResults"), host);
    lbMax->setObjectName(QStringLiteral("label_maxDownloadNum")); lbMax->setWordWrap(true);
    g->addWidget(lbMax, 6, 0);
    QLineEdit *edMax = new QLineEdit(host);
    edMax->setObjectName(QStringLiteral("ledt_maxDownloadNum"));
    edMax->setMaxLength(3);
    edMax->setText(QStringLiteral("99"));   // реф. дефолт
    edMax->setValidator(new QRegExpValidator(
        QRegExp(QStringLiteral("^([0-9]|[1-9][0-9]|[1-9][0-9][0-9])$")), edMax));  // 0..999
    g->addWidget(edMax, 6, 1);

    // (7) Описание — статичный список.
    QLabel *lbRPD = new QLabel(tr("TR_SDescription"), host);
    lbRPD->setObjectName(QStringLiteral("label_RPD"));
    lbRPD->setMinimumWidth(100); lbRPD->setWordWrap(true);
    g->addWidget(lbRPD, 7, 0);
    QComboBox *cmbRPD = new QComboBox(host);
    cmbRPD->setObjectName(QStringLiteral("cmb_RPD"));
    cmbRPD->addItem(tr("TR_Inf"));
    cmbRPD->addItem(tr("TR_CField1"));
    cmbRPD->addItem(tr("TR_CField2"));
    g->addWidget(cmbRPD, 7, 1);

    root->addLayout(g);
    root->addStretch(1);

    // Подсказка «* обязательное поле» (реф. horizontalLayout: spacer + label_11 + spacer).
    QHBoxLayout *hHint = new QHBoxLayout();
    hHint->setObjectName(QStringLiteral("horizontalLayout"));
    hHint->addStretch(1);
    QLabel *lbHint = new QLabel(tr("TR_RField"), host);
    lbHint->setObjectName(QStringLiteral("label_11"));
    hHint->addWidget(lbHint);
    hHint->addStretch(1);
    root->addLayout(hHint);

    // Кнопки Confirm/Cancel/Reset (реф. horizontalLayout_2, margins 9).
    QHBoxLayout *hBtn = new QHBoxLayout();
    hBtn->setObjectName(QStringLiteral("horizontalLayout_2"));
    hBtn->setContentsMargins(9, 9, 9, 9);
    auto mkBtn = [&](const char *name, const QString &text) {
        QPushButton *b = new QPushButton(text, host);
        b->setObjectName(QString::fromLatin1(name));
        b->setFixedWidth(150);
        hBtn->addWidget(b);
        return b;
    };
    mkBtn("btn_confirm", tr("TR_Cfm"));    // реф. Save (device) — не подключаем
    QPushButton *btnCancel = mkBtn("btn_cancel", tr("TR_Ccl"));
    connect(btnCancel, &QPushButton::clicked, this, &QWidget::close);   // реф. Cancel→close
    mkBtn("btn_reset", tr("TR_Rst2"));     // реф. LoadDefaultData — UI-стуб
    root->addLayout(hBtn);
}
