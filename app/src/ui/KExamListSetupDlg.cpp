#include "KExamListSetupDlg.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QWidget>

KExamListSetupDlg::KExamListSetupDlg(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x7f97e0: KDialog(modal=false) → setupUi → SetKStyle(7) → title TR_MRLSettings →
    // ResetWidgetLayout → LoadConfigData (device) → Initconnect.
    setupUi();
    SetKStyle(KDLG_W1024);            // реф. SetKStyle(7)
    SetTitle(tr("TR_MRLSettings"));
}

void KExamListSetupDlg::setupUi()
{
    setObjectName(QStringLiteral("KExamListSetupDlg"));

    QWidget *host = ContentArea();
    host->setFixedSize(1024, 712);   // реф. фикс. размер, абсолютная геометрия

    // === grp_list: чекбоксы видимости колонок ===
    QGroupBox *grpList = new QGroupBox(host);
    grpList->setObjectName(QStringLiteral("grp_list"));
    grpList->setGeometry(28, 54, 972, 307);
    QLabel *lblList = new QLabel(tr("TR_LDisplay"), grpList);
    lblList->setObjectName(QStringLiteral("label_list"));
    lblList->setGeometry(17, 19, 97, 25);
    auto chk = [&](const char *name, const QString &text, int x, int y) {
        QCheckBox *c = new QCheckBox(text, grpList);
        c->setObjectName(QString::fromLatin1(name));
        c->setGeometry(x, y, 160, 31);
        return c;
    };
    chk("chb_gender", tr("TR_Gdr"), 46, 63);
    chk("chb_age", tr("TR_Age"), 336, 63);
    chk("chb_examdate", tr("TR_EmDate"), 610, 63);
    chk("chb_applicant", tr("TR_Aplct"), 46, 116);
    chk("chb_endo", tr("TR_EModel"), 336, 116);
    chk("chb_endosn", tr("TR_ESN"), 610, 116);
    chk("chb_dob", tr("TR_DoB"), 46, 169);
    chk("chb_tel", tr("TR_Tel"), 336, 170);
    chk("chb_bedno", tr("TR_BNo"), 610, 170);
    chk("chb_register_number", tr("TR_ANumber"), 46, 223);
    chk("chb_useritem1", tr("TR_CField1"), 336, 223);
    chk("chb_useritem2", tr("TR_CField2"), 610, 223);
    QLabel *lblTip = new QLabel(tr("TR_VITMRLWChecked"), grpList);
    lblTip->setObjectName(QStringLiteral("label_tip"));
    lblTip->setGeometry(34, 269, 803, 31);

    // === grp_path: путь экспорта на USB ===
    QGroupBox *grpPath = new QGroupBox(host);
    grpPath->setObjectName(QStringLiteral("grp_path"));
    grpPath->setGeometry(28, 376, 972, 211);
    QLabel *lblPath = new QLabel(tr("TR_EPath"), grpPath);
    lblPath->setObjectName(QStringLiteral("label"));
    lblPath->setGeometry(17, 28, 97, 25);
    QRadioButton *path1 = new QRadioButton(tr("TR_ExportUdiskPath1"), grpPath);
    path1->setObjectName(QStringLiteral("radiobtn_path1"));
    path1->setGeometry(40, 60, 414, 31);
    QRadioButton *path2 = new QRadioButton(tr("TR_ExportUdiskPath2"), grpPath);
    path2->setObjectName(QStringLiteral("radiobtn_path2"));
    path2->setGeometry(40, 100, 414, 31);
    QButtonGroup *grpExport = new QButtonGroup(this);   // реф. btn_group_export (exclusive)
    grpExport->setObjectName(QStringLiteral("btn_group_export"));
    grpExport->addButton(path1);
    grpExport->addButton(path2);
    path1->setChecked(true);

    // === Кнопки ===
    QPushButton *btnSave = new QPushButton(tr("TR_Sve"), host);
    btnSave->setObjectName(QStringLiteral("btn_save"));
    btnSave->setGeometry(360, 643, 133, 40);   // реф. SaveSetupData (device) → close
    connect(btnSave, &QPushButton::clicked, this, &QWidget::close);
    QPushButton *btnExit = new QPushButton(tr("TR_Ext"), host);
    btnExit->setObjectName(QStringLiteral("btn_exit"));
    btnExit->setGeometry(532, 643, 133, 40);
    connect(btnExit, &QPushButton::clicked, this, &QWidget::close);   // реф. ExitSetupData→close
}
