#include "KPasswordEdit.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

KPasswordEdit::KPasswordEdit(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x456048: KDialog(modal=false) → setupUi → SetKStyle(3) → title TR_PEdit →
    // echoMode Password + maxLen16 + validator(GetPasswordRegExp) → btn_ok disabled → connects.
    setupUi();
    SetKStyle(KDLG_W320);           // реф. SetKStyle(3)
    SetTitle(tr("TR_PEdit"));       // реф. перекрывает setupUi-титул TR_Dlg
}

void KPasswordEdit::setupUi()
{
    setObjectName(QStringLiteral("KPasswordEdit"));

    QWidget *host = ContentArea();
    QVBoxLayout *root = new QVBoxLayout(host);

    // Сетка полей пароля.
    QGridLayout *g = new QGridLayout();
    g->setObjectName(QStringLiteral("gridLayout"));
    QLabel *lPw = new QLabel(tr("TR_NPassword:"), host);
    lPw->setObjectName(QStringLiteral("label_passwd"));
    lPw->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    g->addWidget(lPw, 0, 0);
    QLineEdit *pw = new QLineEdit(host);   // реф. KPasswordLineEdit → QLineEdit
    pw->setObjectName(QStringLiteral("password"));
    pw->setEchoMode(QLineEdit::Password);
    pw->setMaxLength(16);
    pw->setToolTip(tr("TR_IAMCharacters"));
    g->addWidget(pw, 0, 1);
    QLabel *lCf = new QLabel(tr("TR_Cfm:"), host);
    lCf->setObjectName(QStringLiteral("label_passwdcf"));
    lCf->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    g->addWidget(lCf, 1, 0);
    QLineEdit *cf = new QLineEdit(host);
    cf->setObjectName(QStringLiteral("passwdconfirm"));
    cf->setEchoMode(QLineEdit::Password);
    cf->setMaxLength(16);
    cf->setToolTip(tr("TR_IAMCharacters"));
    g->addWidget(cf, 1, 1);
    root->addLayout(g);

    root->addStretch(1);

    // Ряд кнопок.
    QHBoxLayout *h = new QHBoxLayout();
    h->setObjectName(QStringLiteral("horizontalLayout"));
    h->addStretch(1);
    QToolButton *btnOk = new QToolButton(host);
    btnOk->setObjectName(QStringLiteral("btn_ok"));
    btnOk->setText(tr("TR_OK"));
    btnOk->setEnabled(false);   // реф. изначально выключена
    h->addWidget(btnOk);
    h->addStretch(1);
    QToolButton *btnCancel = new QToolButton(host);
    btnCancel->setObjectName(QStringLiteral("btn_cancel"));
    btnCancel->setText(tr("TR_Ccl"));
    h->addWidget(btnCancel);
    h->addStretch(1);
    root->addLayout(h);

    // Реф. OnCheckInput — OK активна только когда оба поля непусты (чистый UI).
    auto checkInput = [pw, cf, btnOk]() {
        btnOk->setEnabled(!pw->text().isEmpty() && !cf->text().isEmpty());
    };
    connect(pw, &QLineEdit::textChanged, this, checkInput);
    connect(cf, &QLineEdit::textChanged, this, checkInput);
    connect(btnCancel, &QToolButton::clicked, this, &QWidget::close);   // реф. Cancel→close
    // Реф. Save: IsPasswordConfirmSame (сверка — чистый UI) → device (MD5/SaveAccount) → close.
    connect(btnOk, &QToolButton::clicked, this, [this, pw, cf]() {
        if (pw->text() != cf->text())
            return;   // реф. IsPasswordConfirmSame: при несовпадении — warning, не сохраняем
        close();      // реф. device-часть (ValidateIfPWValid/ConvertPasswordToMD5/SaveAccount) — заглушка
    });
}
