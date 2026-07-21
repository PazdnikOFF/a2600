#include "KProcessorSN.h"

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegExpValidator>
#include <QSpacerItem>
#include <QWidget>

KProcessorSN::KProcessorSN(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x6ff748: KDialog(modal=false) → setupUi → SetKStyle(2) → title TR_PSN →
    // validator [A-Za-z0-9]{0,12} → connect(OK→onOK, Cancel→close).
    setupUi();
    SetKStyle(KDLG_W460);              // реф. SetKStyle(2)
    SetTitle(tr("TR_PSN"));
}

void KProcessorSN::setupUi()
{
    setObjectName(QStringLiteral("KProcessorSN"));
    resize(640, 480);

    QWidget *host = ContentArea();
    QGridLayout *g3 = new QGridLayout(host);   // реф. gridLayout_3 — центрирующая сетка
    g3->setObjectName(QStringLiteral("gridLayout_3"));
    g3->addItem(new QSpacerItem(20, 144, QSizePolicy::Minimum, QSizePolicy::Expanding), 0, 1);
    g3->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum), 1, 0);
    g3->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum), 1, 2);
    g3->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding), 2, 1);
    g3->addItem(new QSpacerItem(20, 143, QSizePolicy::Minimum, QSizePolicy::Expanding), 4, 1);

    // Внутренняя сетка контента.
    QGridLayout *g = new QGridLayout();
    g->setObjectName(QStringLiteral("gridLayout"));
    QLabel *lblTitle = new QLabel(tr("TR_PITPSN:"), host);
    lblTitle->setObjectName(QStringLiteral("label"));
    lblTitle->setMinimumSize(280, 32);
    g->addWidget(lblTitle, 0, 0, 1, 2);
    QLabel *lblSN = new QLabel(QStringLiteral("SN:"), host);   // реф. литерал, не TR
    lblSN->setObjectName(QStringLiteral("label_2"));
    lblSN->setMinimumSize(40, 32);
    lblSN->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    g->addWidget(lblSN, 1, 0);
    QLineEdit *edit = new QLineEdit(host);
    edit->setObjectName(QStringLiteral("lineEdit"));
    edit->setValidator(new QRegExpValidator(
        QRegExp(QStringLiteral("[A-Za-z0-9]{0,12}")), edit));   // реф. валидатор
    g->addWidget(edit, 1, 1);
    g3->addLayout(g, 1, 1);

    // Ряд кнопок.
    QGridLayout *g2 = new QGridLayout();
    g2->setObjectName(QStringLiteral("gridLayout_2"));
    QPushButton *btnOk = new QPushButton(tr("TR_OK"), host);
    btnOk->setObjectName(QStringLiteral("btn_OK"));
    btnOk->setFixedWidth(100);
    QPushButton *btnCancel = new QPushButton(tr("TR_Ccl"), host);
    btnCancel->setObjectName(QStringLiteral("btn_Cancel"));
    btnCancel->setFixedWidth(100);
    g2->addWidget(btnOk, 0, 0);
    g2->addWidget(btnCancel, 0, 1);
    g3->addLayout(g2, 3, 1);

    // Реф. onOK: валидный SN (>8 симв.) → сохранить в m_SN + accept; иначе warning. У нас —
    // закрываем при SN>8 (валидация — UI); getter GetSetSN/device-запись — в вызывающем коде.
    connect(btnOk, &QPushButton::clicked, this, [this, edit]() {
        if (edit->text().trimmed().length() > 8)
            close();
    });
    connect(btnCancel, &QPushButton::clicked, this, &QWidget::close);   // реф. Cancel→close
}
