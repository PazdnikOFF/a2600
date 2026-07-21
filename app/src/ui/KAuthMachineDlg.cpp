#include "KAuthMachineDlg.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <QVBoxLayout>
#include <QWidget>

namespace {
// Строка «подпись : значение» (значение пусто — заполняется устройством из KSystemSet).
// Реф.: caption minW 300, expanding; value align 0x81 (Left|VCenter), stretch 1.
QHBoxLayout *rowCapVal(QWidget *p, const QString &cap, const char *valName)
{
    QHBoxLayout *h = new QHBoxLayout();
    h->setContentsMargins(20, 0, 0, 0);   // реф. left margin 20
    QLabel *c = new QLabel(p);
    c->setText(cap);
    c->setMinimumWidth(300);
    c->setWordWrap(true);
    h->addWidget(c);
    QLabel *v = new QLabel(p);
    v->setObjectName(QString::fromLatin1(valName));
    v->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    h->addWidget(v, 1);
    return h;
}
} // namespace

KAuthMachineDlg::KAuthMachineDlg(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x5d5ba0: setupUi → SetKStyle(2) → InitWidgets (device) → InitConnections.
    // Титул окна в реф. — TR_Dlg; ставим осмысленный TR_TMAuthorization в титул-бар KDialog.
    setupUi();
    SetKStyle(KDLG_W460);                  // реф. SetKStyle(2)
    SetTitle(tr("TR_TMAuthorization"));
}

void KAuthMachineDlg::setupUi()
{
    setObjectName(QStringLiteral("KAuthMachineDlg"));

    QWidget *host = ContentArea();
    QVBoxLayout *root = new QVBoxLayout(host);
    root->setObjectName(QStringLiteral("gridLayout_2"));

    // === Группа авторизации ===
    QGroupBox *groupBox_3 = new QGroupBox(host);
    groupBox_3->setObjectName(QStringLiteral("groupBox_3"));
    groupBox_3->setTitle(tr("TR_TMAuthorization"));
    QVBoxLayout *v = new QVBoxLayout(groupBox_3);
    v->setObjectName(QStringLiteral("verticalLayout"));
    v->setContentsMargins(9, 25, 9, 9);    // реф. top margin 25

    // Четыре строки значений (заполнение — device, поля пусты).
    v->addLayout(rowCapVal(groupBox_3, tr("TR_PSN:"), "label_SN"));           // серийник процессора
    v->addLayout(rowCapVal(groupBox_3, tr("TR_PCN:"), "label_CN"));           // CN изделия
    v->addLayout(rowCapVal(groupBox_3, tr("TR_EDate:"), "label_RemainTimes_2")); // срок действия
    v->addLayout(rowCapVal(groupBox_3, tr("TR_Rmnng:"), "label_RemainTimes_3")); // остаток дней

    // Блок из двух кнопок-действий (реф. gridLayout, margins 90/9/90, minW 420).
    QGridLayout *g = new QGridLayout();
    g->setObjectName(QStringLiteral("gridLayout"));
    g->setContentsMargins(90, 9, 90, 9);
    QPushButton *btnAuth = new QPushButton(groupBox_3);
    btnAuth->setObjectName(QStringLiteral("btn_authMachine"));
    btnAuth->setText(tr("TR_ETMachine"));  // реф. дефолт (неавторизовано); авторизовано → TR_DTMachine
    btnAuth->setMinimumWidth(420);
    g->addWidget(btnAuth, 0, 0);
    QPushButton *btnServer = new QPushButton(groupBox_3);
    btnServer->setObjectName(QStringLiteral("btn_serverset"));
    btnServer->setText(tr("TR_SSettings2"));
    btnServer->setMinimumWidth(420);
    g->addWidget(btnServer, 1, 0);
    v->addLayout(g);
    // btn_authMachine → ClickAuthMachine (DES/KControlProc) — DEVICE, не подключаем.
    // btn_serverset  → SetServerInfo — DEVICE, не подключаем.

    root->addWidget(groupBox_3);

    // Реф. вертикальный спейсер между блоком кнопок и строкой Exit.
    root->addStretch(1);

    // === Exit (реф. horizontalLayout_26, sizeConstraint SetFixedSize; btn fixed 160) ===
    QHBoxLayout *hexit = new QHBoxLayout();
    hexit->setObjectName(QStringLiteral("horizontalLayout_26"));
    hexit->addStretch(1);
    QPushButton *btnExit = new QPushButton(host);
    btnExit->setObjectName(QStringLiteral("btn_exit_2"));
    btnExit->setText(tr("TR_Ext"));
    btnExit->setFixedWidth(160);
    connect(btnExit, &QPushButton::clicked, this, &QWidget::close);   // реф. Exit()→close
    hexit->addWidget(btnExit);
    hexit->addStretch(1);
    root->addLayout(hexit);
}
