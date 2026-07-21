#include "KAddPrinterDlg.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegExpValidator>
#include <QSpacerItem>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

KAddPrinterDlg::KAddPrinterDlg(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x7972f0 → Init: KDialog(modal=false) → setupUi → SetTitle(TR_APrinter) →
    // setStyleSheet(тёмный) → validator/стили → 2 stacked-страницы → connects → прайм страницы.
    setupUi();
    SetTitle(tr("TR_APrinter"));
}

void KAddPrinterDlg::setupUi()
{
    setObjectName(QStringLiteral("KAddPrinterDlg"));
    resize(828, 381);

    QWidget *host = ContentArea();
    host->setStyleSheet(QStringLiteral("QWidget{background-color: rgba(26,26,26,255);}"));
    QVBoxLayout *root = new QVBoxLayout(host);
    root->setContentsMargins(10, 45, 10, 10);
    root->setSpacing(15);

    QGridLayout *g = new QGridLayout();
    g->setSpacing(10);
    auto capLbl = [&](const QString &text, const char *name) {
        QLabel *l = new QLabel(text, host);
        l->setObjectName(QString::fromLatin1(name));
        l->setFixedSize(200, 38);
        l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        return l;
    };
    auto actBtn = [&](const char *name, const QString &text) {
        QPushButton *b = new QPushButton(text, host);
        b->setObjectName(QString::fromLatin1(name));
        b->setFixedSize(150, 31);
        return b;
    };

    // (0) Имя сервиса.
    g->addWidget(capLbl(tr("TR_PName"), "m_pServiceNameLbl"), 0, 0);
    QLineEdit *edName = new QLineEdit(host);
    edName->setObjectName(QStringLiteral("m_pServiceNameLedit"));
    edName->setFixedWidth(360);
    edName->setValidator(new QRegExpValidator(QRegExp(QStringLiteral("[A-Z_a-z0-9-]{1,16}")), edName));
    g->addWidget(edName, 0, 1);
    g->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding), 0, 2, 2, 1);

    // (1) Тип подключения (статичный combo).
    g->addWidget(capLbl(tr("TR_CType2"), "m_pConnectTypeLbl"), 1, 0);
    QComboBox *cmbType = new QComboBox(host);
    cmbType->setObjectName(QStringLiteral("m_pConnectTypeCmb"));
    cmbType->setFixedWidth(360);
    cmbType->addItem(tr("TR_WPrinter"));   // реф. статично 0/1/2
    cmbType->addItem(tr("TR_UPrinter"));
    cmbType->addItem(tr("TR_NPrinter"));
    g->addWidget(cmbType, 1, 1);

    // (2) Адрес устройства (QStackedWidget: URL-combo / IP-edit) + Search.
    g->addWidget(capLbl(tr("TR_DIPAddress"), "m_pDeviceIPLbl"), 2, 0);
    QStackedWidget *stk = new QStackedWidget(host);
    stk->setObjectName(QStringLiteral("m_pStackedWdt"));
    stk->setFixedWidth(360);
    QComboBox *urlCombo = new QComboBox(stk);   // page0: URL/принтер (device-populated)
    urlCombo->setEditable(true);
    stk->addWidget(urlCombo);
    QLineEdit *ipEdit = new QLineEdit(stk);     // page1: KIpLineEdit → QLineEdit+маска
    ipEdit->setInputMask(QStringLiteral("000.000.000.000;_"));
    stk->addWidget(ipEdit);
    g->addWidget(stk, 2, 1);
    g->addWidget(actBtn("m_pSearchBtn", tr("TR_Sch")), 2, 2);   // реф. OnSearchBtnClicked (device)

    // (3) Драйвер (readonly) + AddDriver.
    g->addWidget(capLbl(tr("TR_Drvr"), "m_pDriverLbl"), 3, 0);
    QLineEdit *edDriver = new QLineEdit(host);
    edDriver->setObjectName(QStringLiteral("m_pDriverLedit"));
    edDriver->setFixedWidth(360);
    edDriver->setReadOnly(true);
    edDriver->setFocusPolicy(Qt::NoFocus);   // реф. NoFocus + border-стиль
    edDriver->setStyleSheet(QStringLiteral("QLineEdit{border:1px solid rgb(115,115,115);}"));
    g->addWidget(edDriver, 3, 1);
    g->addWidget(actBtn("m_pAddDriverBtn", tr("TR_Add2")), 3, 2);   // реф. OnAddDriverBtnClicked (под-диалог)
    root->addLayout(g);

    // Ряд чекбоксов принтеров по умолчанию.
    QHBoxLayout *hCk = new QHBoxLayout();
    hCk->addStretch(1);
    QCheckBox *ckImg = new QCheckBox(tr("TR_IPService"), host);
    ckImg->setObjectName(QStringLiteral("m_pDefaultImgPrinterCkbox"));
    hCk->addWidget(ckImg);
    hCk->addStretch(1);
    QCheckBox *ckRep = new QCheckBox(tr("TR_RPService"), host);
    ckRep->setObjectName(QStringLiteral("m_pDefaultReportPrinterCkbox"));
    ckRep->setChecked(true);   // реф. default checked
    hCk->addWidget(ckRep);
    hCk->addStretch(1);
    root->addLayout(hCk);

    // Ряд OK/Cancel.
    QHBoxLayout *hBtn = new QHBoxLayout();
    hBtn->addStretch(1);
    QPushButton *btnOk = new QPushButton(tr("TR_OK"), host); btnOk->setObjectName(QStringLiteral("m_pOkBtn"));
    hBtn->addWidget(btnOk);
    hBtn->addStretch(1);
    QPushButton *btnCancel = new QPushButton(tr("TR_Ccl"), host); btnCancel->setObjectName(QStringLiteral("m_pCancelBtn"));
    hBtn->addWidget(btnCancel);
    hBtn->addStretch(1);
    root->addLayout(hBtn);

    // ConnectType переключает stacked-страницу (реф. OnConnectTypeCmbCurrentIndexChanged) — UI.
    connect(cmbType, QOverload<int>::of(&QComboBox::currentIndexChanged), stk,
            [stk](int idx) { stk->setCurrentIndex(idx == 2 ? 1 : 0); });   // Network→IP, иначе URL
    cmbType->setCurrentIndex(0);
    connect(btnCancel, &QPushButton::clicked, this, &QWidget::close);   // реф. OnCancelBtnClicked→close
    // OK/Search/AddDriver/URL-combo — DEVICE (CUPS/принтер-бэкенд), не подключаем.
}
