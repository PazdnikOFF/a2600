#include "KLogin.h"
#include "KPasswordLineEdit.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpacerItem>
#include <QWidget>

KLogin::KLogin(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x6f78d0: setupUi → SetKStyle(2) → title TR_Lgn → SetBtnCloseVisble(false) →
    // echoMode пароля → InitWidget (logout-видимость) → QTimer(1000) → connect'ы. InitAccount/
    // KAccount (device) опущены.
    setupUi();
    SetKStyle(KDLG_W460);            // реф. SetKStyle(2)
    SetTitle(tr("TR_Lgn"));          // реф. перекрывает TR_Dlg
    SetBtnCloseVisible(false);       // реф. SetBtnCloseVisble(false) — без close
    lineEdit_passwd->setEchoMode(QLineEdit::Password);
    lineEdit_id->setFocus();

    connect(btn_login, &QPushButton::clicked, this, &KLogin::Login);
    connect(btn_logout, &QPushButton::clicked, this, &KLogin::Logout);
}

void KLogin::setupUi()
{
    setObjectName(QStringLiteral("KLogin"));
    resize(460, 760);   // реф. 460×1080; уменьшено для читаемого превью (layout гибкий)

    QWidget *host = ContentArea();
    QGridLayout *g2 = new QGridLayout(host);
    g2->setObjectName(QStringLiteral("gridLayout_2"));
    g2->setVerticalSpacing(40);
    g2->setContentsMargins(9, 200, 9, 9);   // реф. top=360 (опускает форму); уменьшено для превью

    label_hints = new QLabel(host);
    label_hints->setObjectName(QStringLiteral("label_hints"));
    label_hints->setWordWrap(true);
    label_hints->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
    label_hints->setText(QString());   // пусто, заполняется в рантайме
    g2->addWidget(label_hints, 0, 0, 1, 3);

    // --- форма (центрирована боковыми спейсерами) ---
    g2->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum), 1, 0);
    QGridLayout *form = new QGridLayout();
    form->setObjectName(QStringLiteral("gridLayout"));
    form->setVerticalSpacing(20);
    QLabel *label_id = new QLabel(host);
    label_id->setObjectName(QStringLiteral("label_id"));
    label_id->setText(tr("TR_Acnt:"));
    label_id->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->addWidget(label_id, 0, 0);
    lineEdit_id = new QLineEdit(host);
    lineEdit_id->setObjectName(QStringLiteral("lineEdit_id"));
    lineEdit_id->setMaxLength(8);
    form->addWidget(lineEdit_id, 0, 1);
    QLabel *label_passwd = new QLabel(host);
    label_passwd->setObjectName(QStringLiteral("label_passwd"));
    label_passwd->setText(tr("TR_Pswd:"));
    label_passwd->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->addWidget(label_passwd, 1, 0);
    lineEdit_passwd = new KPasswordLineEdit(host);   // АПГРЕЙД: реальный KPasswordLineEdit (был QLineEdit)
    lineEdit_passwd->setObjectName(QStringLiteral("lineEdit_passwd"));
    lineEdit_passwd->setMaxLength(16);
    form->addWidget(lineEdit_passwd, 1, 1);
    g2->addLayout(form, 1, 1);
    g2->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum), 1, 2);

    // --- кнопки Login / Logout ---
    QHBoxLayout *hb = new QHBoxLayout();
    hb->setObjectName(QStringLiteral("horizontalLayout"));
    btn_login = new QPushButton(host);
    btn_login->setObjectName(QStringLiteral("btn_login"));
    btn_login->setText(tr("TR_AtLogin"));
    btn_login->setFixedSize(160, 28);
    hb->addWidget(btn_login);
    hb->addStretch(1);
    btn_logout = new QPushButton(host);
    btn_logout->setObjectName(QStringLiteral("btn_logout"));
    btn_logout->setText(tr("TR_ALogin"));
    btn_logout->setFixedSize(160, 28);
    btn_logout->setVisible(false);   // реф. InitWidget: скрыт (если не IsAutoLogin)
    hb->addWidget(btn_logout);
    g2->addLayout(hb, 2, 0, 1, 3);

    g2->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding), 3, 0, 1, 3);
}

void KLogin::Login()  { /* реф.: проверка учётной записи (KAccount) — device */ }
void KLogin::Logout() { /* реф.: выход — device */ }
