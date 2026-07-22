#include "KPUserLoginDlg.h"
#include "KPasswordLineEdit.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegExpValidator>
#include <QVBoxLayout>
#include <QWidget>

KPUserLoginDlg::KPUserLoginDlg(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x5de170: KDialog(modal=false) → setupUi → SetKStyle(2) → InitWidget
    // (echo/focus/maxLen512/validator/eventFilter) → InitConnect. Титул reф. — «Dialog»
    // (мёртвый uic-дефолт); ставим TR_DLogin.
    setupUi();
    SetKStyle(KDLG_W460);              // реф. SetKStyle(2)
    SetTitle(tr("TR_DLogin"));
}

void KPUserLoginDlg::setupUi()
{
    setObjectName(QStringLiteral("KPUserLoginDlg"));
    resize(600, 400);

    QWidget *host = ContentArea();
    QGridLayout *g5 = new QGridLayout(host);

    QGroupBox *grp = new QGroupBox(tr("TR_DLogin"), host);
    grp->setObjectName(QStringLiteral("groupBox"));
    QVBoxLayout *v2 = new QVBoxLayout(grp);
    v2->addStretch(1);

    QVBoxLayout *v = new QVBoxLayout();
    // Строка формы (центрированная спейсерами).
    QHBoxLayout *h = new QHBoxLayout();
    h->addStretch(1);
    QGridLayout *gForm = new QGridLayout();
    gForm->setVerticalSpacing(20);
    QLabel *lblId = new QLabel(tr("TR_Acnt:"), grp);
    lblId->setObjectName(QStringLiteral("label_id"));
    lblId->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    gForm->addWidget(lblId, 0, 0);
    QLineEdit *edAcc = new QLineEdit(grp);
    edAcc->setObjectName(QStringLiteral("lineEdit_account"));
    edAcc->setMaxLength(512);
    gForm->addWidget(edAcc, 0, 1);
    QLabel *lblPw = new QLabel(tr("TR_Pswd:"), grp);
    lblPw->setObjectName(QStringLiteral("label_passwd"));
    lblPw->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    gForm->addWidget(lblPw, 1, 0);
    KPasswordLineEdit *edPw = new KPasswordLineEdit(grp);   // АПГРЕЙД: реальный KPasswordLineEdit
    // (тип конкретный — setValidator у KPasswordLineEdit ПЕРЕОпределён/невиртуальный: берёт QRegExp
    //  из QRegExpValidator для живой фильтрации; QLineEdit* вызвал бы базовый setValidator)
    edPw->setObjectName(QStringLiteral("lineEdit_passwd"));
    edPw->setEchoMode(QLineEdit::Password);
    edPw->setMinimumWidth(180);
    edPw->setMaxLength(512);
    edPw->setValidator(new QRegExpValidator(   // реф. blacklist (без пробелов/пунктуации)
        QRegExp(QStringLiteral("[^\\s_\\-!@#$%&*(){}\\[\\]:;\"'\\\\|?/><,.`~=+]+")), edPw));
    gForm->addWidget(edPw, 1, 1);
    h->addLayout(gForm);
    h->addStretch(1);
    v->addLayout(h);
    v->addSpacing(20);

    // Ряд кнопок.
    QHBoxLayout *hBtn = new QHBoxLayout();
    hBtn->setObjectName(QStringLiteral("horizontalLayout_12"));
    QPushButton *btnLogin = new QPushButton(tr("TR_Lgn"), grp);
    btnLogin->setObjectName(QStringLiteral("btn_login"));
    btnLogin->setFixedWidth(160);   // реф. min=max ширина 160
    QPushButton *btnCancel = new QPushButton(tr("TR_Lgt"), grp);
    btnCancel->setObjectName(QStringLiteral("btn_cancel"));
    btnCancel->setFixedWidth(160);
    hBtn->addWidget(btnLogin);
    hBtn->addStretch(1);
    hBtn->addWidget(btnCancel);
    v->addLayout(hBtn);
    v2->addLayout(v);
    v2->addStretch(1);

    g5->addWidget(grp, 0, 0);

    edAcc->setFocus();   // реф. InitWidget: account получает фокус
    connect(btnCancel, &QPushButton::clicked, this, &QWidget::close);   // реф. CancelLogin→close
    // btn_login → Login() (проверка учётки по БД) — DEVICE, не подключаем.
}
