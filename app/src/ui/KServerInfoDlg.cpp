#include "KServerInfoDlg.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

KServerInfoDlg::KServerInfoDlg(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x5efe60: KDialog(modal=false) → setupUi → SetKStyle(2) → InitWidgets
    // (чтение ini, device) → InitConnections. Титул окна в реф. TR_Dlg; ставим TR_SSettings2.
    setupUi();
    SetKStyle(KDLG_W460);              // реф. SetKStyle(2)
    SetTitle(tr("TR_SSettings2"));
}

void KServerInfoDlg::setupUi()
{
    setObjectName(QStringLiteral("KServerInfoDlg"));
    resize(562, 587);

    QWidget *host = ContentArea();
    QVBoxLayout *root = new QVBoxLayout(host);

    QGroupBox *grp = new QGroupBox(host);
    grp->setObjectName(QStringLiteral("groupBox_3"));
    grp->setTitle(tr("TR_SSettings2"));
    QVBoxLayout *v = new QVBoxLayout(grp);
    v->setObjectName(QStringLiteral("verticalLayout"));
    v->setContentsMargins(9, 25, 9, 9);   // реф. top margin 25

    auto capLbl = [&](const QString &text, const char *name, bool wrap = false) {
        QLabel *l = new QLabel(text, grp);
        l->setObjectName(QString::fromLatin1(name));
        l->setMinimumWidth(240);
        if (wrap) l->setWordWrap(true);
        return l;
    };
    auto editField = [&](const char *name) {
        QLineEdit *e = new QLineEdit(grp);   // реф. значение из ini (device) — пусто
        e->setObjectName(QString::fromLatin1(name));
        e->setMinimumWidth(200);
        return e;
    };

    // DNS1 (реф. label_7 "DNS1:" + label_dns1 QLineEdit).
    QHBoxLayout *h20 = new QHBoxLayout(); h20->setContentsMargins(20, 0, 0, 0);
    h20->addWidget(capLbl(QStringLiteral("DNS1:"), "label_7"));
    h20->addWidget(editField("label_dns1"));
    v->addLayout(h20);
    // DNS2.
    QHBoxLayout *h21 = new QHBoxLayout(); h21->setContentsMargins(20, 0, 0, 0);
    h21->addWidget(capLbl(QStringLiteral("DNS2:"), "label_8"));
    h21->addWidget(editField("label_dns2"));
    v->addLayout(h21);
    // Прокси-сервер: подпись + инертная пустая метка (реф.), поле — ниже.
    QHBoxLayout *h22 = new QHBoxLayout(); h22->setContentsMargins(20, 0, 0, 0);
    h22->addWidget(capLbl(tr("TR_PServer:"), "label_11", /*wrap=*/true));
    QLabel *lblInert = new QLabel(grp);
    lblInert->setObjectName(QStringLiteral("label"));
    lblInert->setEnabled(false);   // реф. setEnabled(false), пустой
    h22->addWidget(lblInert);
    v->addLayout(h22);
    QHBoxLayout *h23 = new QHBoxLayout(); h23->setContentsMargins(20, 0, 0, 0);
    h23->addWidget(editField("label_proxy"));   // реф. прокси/базовый URL
    v->addLayout(h23);

    root->addWidget(grp);
    root->addStretch(1);   // реф. verticalSpacer

    // Кнопки Save/Exit (реф. horizontalLayout_25, min=max 160).
    QHBoxLayout *h25 = new QHBoxLayout();
    h25->setObjectName(QStringLiteral("horizontalLayout_25"));
    QPushButton *btnSave = new QPushButton(tr("TR_Sve"), host);
    btnSave->setObjectName(QStringLiteral("btn_save"));
    btnSave->setFixedWidth(160);
    QPushButton *btnExit = new QPushButton(tr("TR_Ext"), host);
    btnExit->setObjectName(QStringLiteral("btn_exit"));
    btnExit->setFixedWidth(160);
    h25->addStretch(1);
    h25->addWidget(btnSave);
    h25->addWidget(btnExit);
    h25->addStretch(1);
    root->addLayout(h25);

    connect(btnSave, &QPushButton::clicked, this, &QWidget::close);   // реф. Save (device ini/resolv.conf) → close
    connect(btnExit, &QPushButton::clicked, this, &QWidget::close);   // реф. Exit→close
    btnSave->setFocus();   // реф. showEvent: btn_save->setFocus
}
