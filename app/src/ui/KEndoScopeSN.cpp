#include "KEndoScopeSN.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegExpValidator>
#include <QSpacerItem>
#include <QWidget>

KEndoScopeSN::KEndoScopeSN(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x742110: KDialog(modal=false) → setupUi → SetKStyle(2) → title TR_ESN →
    // validator [A-Za-z0-9]{0,12} → connect(OK→onOK, Cancel→close).
    setupUi();
    SetKStyle(KDLG_W460);              // реф. SetKStyle(2)
    SetTitle(tr("TR_ESN"));
}

void KEndoScopeSN::setupUi()
{
    setObjectName(QStringLiteral("KEndoScopeSN"));
    resize(640, 480);

    QWidget *host = ContentArea();
    QGridLayout *g2 = new QGridLayout(host);   // реф. gridLayout_2 — центрирующая сетка 5×3
    g2->setObjectName(QStringLiteral("gridLayout_2"));
    g2->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding), 0, 1);
    g2->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum), 1, 0);
    g2->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum), 1, 2);
    g2->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding), 2, 1);
    g2->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding), 4, 1);

    // Внутренняя сетка контента.
    QGridLayout *g = new QGridLayout();
    g->setObjectName(QStringLiteral("gridLayout"));
    QLabel *lblTitle = new QLabel(tr("TR_PETESNNumber:"), host);
    lblTitle->setObjectName(QStringLiteral("label"));
    lblTitle->setMinimumWidth(280);
    g->addWidget(lblTitle, 0, 0, 1, 2);
    QLabel *lblSN = new QLabel(QStringLiteral("SN:"), host);   // реф. литерал, не TR
    lblSN->setObjectName(QStringLiteral("label_2"));
    lblSN->setMinimumWidth(40);
    lblSN->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    g->addWidget(lblSN, 1, 0);
    QLineEdit *edit = new QLineEdit(host);
    edit->setObjectName(QStringLiteral("lineEdit"));
    edit->setValidator(new QRegExpValidator(
        QRegExp(QStringLiteral("[A-Za-z0-9]{0,12}")), edit));   // реф. валидатор
    g->addWidget(edit, 1, 1);
    g2->addLayout(g, 1, 1);

    // Ряд кнопок.
    QHBoxLayout *h = new QHBoxLayout();
    h->setObjectName(QStringLiteral("horizontalLayout"));
    QPushButton *btnOk = new QPushButton(tr("TR_OK"), host);
    btnOk->setObjectName(QStringLiteral("btn_OK"));
    btnOk->setFixedWidth(100);
    QPushButton *btnCancel = new QPushButton(tr("TR_Ccl"), host);
    btnCancel->setObjectName(QStringLiteral("btn_Cancel"));
    btnCancel->setFixedWidth(100);
    h->addWidget(btnOk);
    h->addWidget(btnCancel);
    g2->addLayout(h, 3, 1);

    // Реф. onOK: валидный SN (>8 симв.) → commit(device)+close; иначе warning. У нас —
    // закрываем при непустом SN (пустой/короткий не закрывает); device-запись опущена.
    connect(btnOk, &QPushButton::clicked, this, [this, edit]() {
        if (edit->text().trimmed().length() > 8)
            close();
    });
    connect(btnCancel, &QPushButton::clicked, this, &QWidget::close);   // реф. Cancel→close
}
