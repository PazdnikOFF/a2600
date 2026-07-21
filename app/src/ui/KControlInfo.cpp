#include "KControlInfo.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <QVBoxLayout>
#include <QWidget>

namespace {
// Строка «подпись : значение» (значение пусто — заполняется устройством).
QHBoxLayout *rowCapVal(QWidget *p, const QString &cap, const char *valName,
                       QLabel **outVal = nullptr)
{
    QHBoxLayout *h = new QHBoxLayout();
    QLabel *c = new QLabel(p);
    c->setText(cap);
    c->setMaximumWidth(320);
    c->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    h->addWidget(c);
    QLabel *v = new QLabel(p);
    v->setObjectName(QString::fromLatin1(valName));
    v->setAlignment(Qt::AlignCenter);
    h->addWidget(v, 1);
    if (outVal)
        *outVal = v;
    return h;
}
QPushButton *mkBtn120(QWidget *p, const char *name, const QString &text)
{
    QPushButton *b = new QPushButton(p);
    b->setObjectName(QString::fromLatin1(name));
    b->setText(text);
    b->setFixedWidth(120);
    return b;
}
} // namespace

KControlInfo::KControlInfo(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x707d48: setupUi → SetKStyle(2) → title TR_TTInformation →
    // InitGroupProcessorCtrl/InitGroupEndoCtrl (device) → AllConnect.
    setupUi();
    SetKStyle(KDLG_W460);              // реф. SetKStyle(2)
    SetTitle(tr("TR_TTInformation"));  // реф. титул-бар (перекрывает TR_Dlg)
}

void KControlInfo::setupUi()
{
    setObjectName(QStringLiteral("KControlInfo"));

    QWidget *host = ContentArea();
    QVBoxLayout *root = new QVBoxLayout(host);
    root->setObjectName(QStringLiteral("verticalLayout_3"));

    // === GroupBox Processor ===
    QGroupBox *groupBox = new QGroupBox(host);
    groupBox->setObjectName(QStringLiteral("groupBox"));
    groupBox->setTitle(tr("TR_Prcssr"));
    QVBoxLayout *v2 = new QVBoxLayout(groupBox);
    v2->addLayout(rowCapVal(groupBox, QStringLiteral("SN:"), "label_ProcessorSN"));
    v2->addLayout(rowCapVal(groupBox, tr("TR_EDate:"), "label_Date"));
    v2->addLayout(rowCapVal(groupBox, tr("TR_Rmnng:"), "label_Remain"));
    {
        QHBoxLayout *h = new QHBoxLayout();
        QLabel *c = new QLabel(tr("TR_NOCEndoscopes:"), groupBox);
        c->setMaximumWidth(320); c->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        h->addWidget(c);
        QLabel *v = new QLabel(groupBox); v->setObjectName(QStringLiteral("label_supportEndos"));
        v->setAlignment(Qt::AlignCenter);
        h->addWidget(v, 1);
        h->addWidget(mkBtn120(groupBox, "btn_viewMatchEndo", tr("TR_Vw")));
        v2->addLayout(h);
    }
    {
        QHBoxLayout *h = new QHBoxLayout();
        h->addWidget(mkBtn120(groupBox, "btn_activeProcesssor", tr("TR_Actvte")));
        h->addStretch(1);
        v2->addLayout(h);
    }
    root->addWidget(groupBox);

    // === GroupBox Endoscope ===
    QGroupBox *groupBox_2 = new QGroupBox(host);
    groupBox_2->setObjectName(QStringLiteral("groupBox_2"));
    groupBox_2->setTitle(tr("TR_Escpe"));
    QVBoxLayout *v = new QVBoxLayout(groupBox_2);
    v->addLayout(rowCapVal(groupBox_2, QStringLiteral("SN:"), "label_EndoSN"));
    v->addLayout(rowCapVal(groupBox_2, tr("TR_EDate:"), "label_endo_deadline"));
    v->addLayout(rowCapVal(groupBox_2, tr("TR_NORUses:"), "label_RemainTimes"));
    {
        QHBoxLayout *h = new QHBoxLayout();
        QLabel *c = new QLabel(tr("TR_CPAmount:"), groupBox_2);
        c->setMaximumWidth(320); c->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        h->addWidget(c);
        QLabel *lv = new QLabel(groupBox_2); lv->setObjectName(QStringLiteral("label_supportProcessor"));
        lv->setAlignment(Qt::AlignCenter);
        h->addWidget(lv, 1);
        h->addWidget(mkBtn120(groupBox_2, "btn_viewMatchPro", tr("TR_Vw")));
        v->addLayout(h);
    }
    {
        QHBoxLayout *h = new QHBoxLayout();
        h->addWidget(mkBtn120(groupBox_2, "btn_activeEndo", tr("TR_Actvte")));
        h->addStretch(1);
        v->addLayout(h);
    }
    root->addWidget(groupBox_2);

    root->addStretch(1);

    // === Exit (центрирован) ===
    QHBoxLayout *hexit = new QHBoxLayout();
    hexit->addStretch(1);
    QPushButton *btn_exit = mkBtn120(host, "btn_exit", tr("TR_Ext"));
    connect(btn_exit, &QPushButton::clicked, this, &QWidget::close);
    hexit->addWidget(btn_exit);
    hexit->addStretch(1);
    root->addLayout(hexit);
}
