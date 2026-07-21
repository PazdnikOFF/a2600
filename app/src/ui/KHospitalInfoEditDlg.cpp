#include "KHospitalInfoEditDlg.h"

#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

KHospitalInfoEditDlg::KHospitalInfoEditDlg(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x559790: KDialog(modal=false) → setupUi → eventFilter → InitWidget
    // (SetTitle TR_EHInformation, стили, LoadDataFromReportTemplateConfig — device) →
    // InitQuickInput → InitConnect.
    setupUi();
    SetTitle(tr("TR_EHInformation"));   // реф. перекрывает setupUi-титул TR_HIEdit
}

void KHospitalInfoEditDlg::setupUi()
{
    setObjectName(QStringLiteral("KHospitalInfoEditDlg"));
    resize(674, 540);

    QWidget *host = ContentArea();
    QVBoxLayout *root = new QVBoxLayout(host);
    root->setContentsMargins(69, 38, 69, 48);   // реф. gridLayout_2 margins
    root->setSpacing(2);

    QGridLayout *g = new QGridLayout();
    g->setObjectName(QStringLiteral("gridLayout"));
    g->setHorizontalSpacing(40);
    g->setVerticalSpacing(24);
    auto capTitle = [&](const QString &text, const char *name) {
        QLabel *l = new QLabel(text, host);
        l->setObjectName(QString::fromLatin1(name));
        l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);   // реф. align 0x82
        return l;
    };

    // (0) Логотип.
    g->addWidget(capTitle(tr("TR_Lgo") + QStringLiteral(":"), "label_logo_title"), 0, 0);
    QHBoxLayout *hLogo = new QHBoxLayout();
    QLabel *logo = new QLabel(host);
    logo->setObjectName(QStringLiteral("label_logo_img"));
    logo->setMaximumSize(432, 68);
    logo->setStyleSheet(QStringLiteral("QLabel#label_logo_img{background-color:white;}"));   // реф. + pixmap device
    hLogo->addWidget(logo);
    hLogo->addStretch(1);
    g->addLayout(hLogo, 0, 1);

    // (1) Заголовок 1 (имя больницы, config).
    g->addWidget(capTitle(tr("TR_Cptn1") + QStringLiteral(":"), "label_cap1_title"), 1, 0);
    QLabel *cap1 = new QLabel(host);
    cap1->setObjectName(QStringLiteral("label_cap1_val"));
    cap1->setMinimumHeight(34);
    cap1->setStyleSheet(QStringLiteral("background-color:rgb(39,39,41);"));   // реф.; текст — config
    g->addWidget(cap1, 1, 1);

    // (2) Заголовок 2 (memory-combo).
    g->addWidget(capTitle(tr("TR_Cptn2") + QStringLiteral(":"), "label_cap2_title"), 2, 0);
    QComboBox *cap2 = new QComboBox(host);   // реф. KMemComboBox → editable QComboBox
    cap2->setObjectName(QStringLiteral("comboBox_caption2"));
    cap2->setEditable(true);
    cap2->setMinimumHeight(34);
    cap2->setStyleSheet(QStringLiteral("background-color:rgb(62,63,68);"));
    if (cap2->lineEdit())
        cap2->lineEdit()->setMaxLength(30);   // реф. maxLen 30
    g->addWidget(cap2, 2, 1);

    // (3) Декларация.
    g->addWidget(capTitle(tr("TR_Dcltn") + QStringLiteral(":"), "label_statement_title"), 3, 0);
    QTextEdit *stmt = new QTextEdit(host);
    stmt->setObjectName(QStringLiteral("textEdit_statement"));
    stmt->setMinimumHeight(109); stmt->setMaximumHeight(109);
    stmt->setStyleSheet(QStringLiteral("background-color:rgb(39,39,41);"));   // текст — config
    g->addWidget(stmt, 3, 1);

    root->addLayout(g);

    // Ряд кнопок.
    QHBoxLayout *h7 = new QHBoxLayout();
    h7->setObjectName(QStringLiteral("horizontalLayout_7"));
    h7->setSpacing(32);
    h7->setContentsMargins(0, 20, 0, 8);
    auto mkBtn = [&](const char *name, const QString &text) {
        QPushButton *b = new QPushButton(text, host);
        b->setObjectName(QString::fromLatin1(name));
        b->setFixedSize(156, 42);
        h7->addWidget(b);
        return b;
    };
    mkBtn("btn_default", tr("TR_Dflt"));   // реф. OnBtnDefaultClicked (config-дефолт) — device
    mkBtn("btn_save", tr("TR_Sve"));       // реф. OnBtnSaveClicked (persist) — device
    QPushButton *btnCancel = mkBtn("btn_cancel", tr("TR_Ext"));
    connect(btnCancel, &QPushButton::clicked, this, &QWidget::close);   // реф. OnBtnCancelClicked→close
    root->addLayout(h7);
}
