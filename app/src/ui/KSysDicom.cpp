#include "KSysDicom.h"

#include <QCheckBox>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpacerItem>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

KSysDicom::KSysDicom(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. Create() @0x5c0928: setupUi → frameless → theme qss → setGeometry(230,160,1909,1059)
    // → LoadData/RefreshDataToUI/RegisterSignalConnect. У нас размер умереннее (layout гибкий).
    setupUi();
    resize(1100, 640);
}

void KSysDicom::setupUi()
{
    setObjectName(QStringLiteral("KSysDicom"));

    QWidget *host = ContentArea();
    QGridLayout *g2 = new QGridLayout(host);
    g2->setObjectName(QStringLiteral("gridLayout_2"));
    g2->setSpacing(0);
    g2->setContentsMargins(0, 0, 0, 0);

    QFrame *frame = new QFrame(host);
    frame->setObjectName(QStringLiteral("frame_DICOMSet"));
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setFrameShadow(QFrame::Raised);
    g2->addWidget(frame, 0, 0);
    QVBoxLayout *vmain = new QVBoxLayout(frame);

    // --- заголовок (внутрирамочный) ---
    QLabel *label_7 = new QLabel(frame);
    label_7->setObjectName(QStringLiteral("label_7"));
    label_7->setText(tr("TR_DSetting"));
    label_7->setAlignment(Qt::AlignCenter);
    label_7->setStyleSheet(QStringLiteral("font-weight:bold;font-size:20px;"));
    vmain->addWidget(label_7);

    auto mkLineEdit = [](QWidget *p, const char *name) {
        QLineEdit *e = new QLineEdit(p);
        e->setObjectName(QString::fromLatin1(name));
        e->setFixedWidth(200);
        return e;
    };

    // --- groupBox: Basic settings ---
    QGroupBox *groupBox = new QGroupBox(frame);
    groupBox->setObjectName(QStringLiteral("groupBox"));
    QVBoxLayout *v4 = new QVBoxLayout(groupBox);
    QLabel *label_5 = new QLabel(groupBox);
    label_5->setObjectName(QStringLiteral("label_5"));
    label_5->setText(tr("TR_BSettings"));
    v4->addWidget(label_5);
    QGridLayout *gform = new QGridLayout();
    QLabel *l1 = new QLabel(tr("TR_SnName:"), groupBox); l1->setObjectName(QStringLiteral("label"));
    gform->addWidget(l1, 0, 1);
    gform->addWidget(mkLineEdit(groupBox, "ledt_WorkStationName"), 0, 3);
    QLabel *l3 = new QLabel(tr("TR_Prt:"), groupBox); l3->setObjectName(QStringLiteral("label_3"));
    gform->addWidget(l3, 0, 5);
    gform->addWidget(mkLineEdit(groupBox, "ledt_localConnPort"), 0, 7);
    QLabel *l2 = new QLabel(tr("TR_LATitle:"), groupBox); l2->setObjectName(QStringLiteral("label_2"));
    gform->addWidget(l2, 1, 1);
    gform->addWidget(mkLineEdit(groupBox, "ledt_localAE"), 1, 3);
    QLabel *l4 = new QLabel(tr("TR_Tmt:"), groupBox); l4->setObjectName(QStringLiteral("label_4"));
    gform->addWidget(l4, 1, 5);
    gform->addWidget(mkLineEdit(groupBox, "ledt_ConnTimeout"), 1, 7);
    gform->setColumnStretch(8, 1);
    v4->addLayout(gform);
    vmain->addWidget(groupBox);

    // --- groupBox_2: Storage settings ---
    QGroupBox *groupBox_2 = new QGroupBox(frame);
    groupBox_2->setObjectName(QStringLiteral("groupBox_2"));
    QVBoxLayout *v3 = new QVBoxLayout(groupBox_2);
    QLabel *label_8 = new QLabel(groupBox_2);
    label_8->setObjectName(QStringLiteral("label_8"));
    label_8->setText(tr("TR_SSetting"));
    v3->addWidget(label_8);
    QHBoxLayout *h6 = new QHBoxLayout();
    QCheckBox *chb1 = new QCheckBox(tr("TR_SPSUpload"), groupBox_2);
    chb1->setObjectName(QStringLiteral("chb_storageSyncUpload"));
    QCheckBox *chb2 = new QCheckBox(tr("TR_UPROIISaved"), groupBox_2);
    chb2->setObjectName(QStringLiteral("chb_PDFRepSyncUpload"));
    h6->addWidget(chb1);
    h6->addSpacing(200);
    h6->addWidget(chb2);
    h6->addStretch(1);
    v3->addLayout(h6);
    vmain->addWidget(groupBox_2);

    // --- groupBox_3: Server list ---
    QGroupBox *groupBox_3 = new QGroupBox(frame);
    groupBox_3->setObjectName(QStringLiteral("groupBox_3"));
    QVBoxLayout *v2 = new QVBoxLayout(groupBox_3);
    QLabel *label_6 = new QLabel(groupBox_3);
    label_6->setObjectName(QStringLiteral("label_6"));
    label_6->setText(tr("TR_SList"));
    v2->addWidget(label_6);
    QTableWidget *tab = new QTableWidget(groupBox_3);
    tab->setObjectName(QStringLiteral("tabView_DICOMSrv"));
    tab->setColumnCount(4);   // реф. наполняет из KDICOMConf (device); заголовки для наглядности
    tab->setHorizontalHeaderLabels({QStringLiteral("AE Title"), QStringLiteral("Host"),
                                    QStringLiteral("Port"), QStringLiteral("Service")});
    v2->addWidget(tab, 1);
    QHBoxLayout *hbtn = new QHBoxLayout();
    auto mkBtn = [&](const char *name, const QString &text) {
        QPushButton *b = new QPushButton(groupBox_3);
        b->setObjectName(QString::fromLatin1(name));
        b->setText(text);
        hbtn->addWidget(b);
        return b;
    };
    mkBtn("pbt_addDICOMItem", tr("TR_AService"));
    mkBtn("pbt_editDICOMItem", tr("TR_EService"));
    mkBtn("pbt_delDICOMItem", tr("TR_Del"));
    mkBtn("pbt_PingDICOMItem", QStringLiteral("Ping"));   // реф. литерал
    mkBtn("pbt_echoDICOMItem", tr("TR_Vrfctn"));
    QLabel *lchk = new QLabel(groupBox_3);
    lchk->setObjectName(QStringLiteral("label_checkDICOMItem"));   // статус (пусто)
    hbtn->addWidget(lchk);
    mkBtn("pbt_loadDefault", tr("TR_Dflt"));
    mkBtn("pbt_save", tr("TR_Sve"));
    QPushButton *pbt_exit = mkBtn("pbt_exit", tr("TR_Ext"));
    connect(pbt_exit, &QPushButton::clicked, this, &QWidget::close);   // реф. Exit()→close
    hbtn->addStretch(1);
    v2->addLayout(hbtn);
    vmain->addWidget(groupBox_3, 1);
}
