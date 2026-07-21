#include "KGeneralSetDlg.h"

#include <QCheckBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

namespace {
// Реф. KLineH — кастомный горизонтальный сепаратор (QFrame HLine/Sunken). У нас QFrame.
QFrame *mkSep(QWidget *p)
{
    QFrame *f = new QFrame(p);
    f->setFrameShape(QFrame::HLine);
    f->setFrameShadow(QFrame::Sunken);
    return f;
}
QLabel *mkHeader(QWidget *p, const QString &text)
{
    QLabel *l = new QLabel(p);
    l->setText(text);
    l->setStyleSheet(QStringLiteral("font-weight:bold;"));
    return l;
}
} // namespace

KGeneralSetDlg::KGeneralSetDlg(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x5eada0: setupUi → SetKStyle(2) → title TR_Gnrl4 → InitWidget → LoadSystemConf
    // (device) → InitConnect → PatientInfoChanged.
    setupUi();
    SetKStyle(KDLG_W460);          // реф. SetKStyle(2)
    SetTitle(tr("TR_Gnrl4"));      // реф. перекрывает "Dialog"
}

void KGeneralSetDlg::setupUi()
{
    setObjectName(QStringLiteral("KGeneralSetDlg"));
    resize(400, 785);

    QWidget *host = ContentArea();
    QVBoxLayout *root = new QVBoxLayout(host);
    root->setObjectName(QStringLiteral("verticalLayout_5"));

    auto addChk = [](QWidget *p, const char *name, const QString &text) {
        QCheckBox *c = new QCheckBox(p);
        c->setObjectName(QString::fromLatin1(name));
        c->setText(text);
        return c;
    };
    auto addRadio = [](QWidget *p, const char *name, const QString &text) {
        QRadioButton *r = new QRadioButton(p);
        r->setObjectName(QString::fromLatin1(name));
        r->setText(text);
        return r;
    };

    // === A. Больница ===
    QFrame *frame_hospitalInfo = new QFrame(host);
    frame_hospitalInfo->setObjectName(QStringLiteral("frame_hospitalInfo"));
    QVBoxLayout *vA = new QVBoxLayout(frame_hospitalInfo);
    QHBoxLayout *hA0 = new QHBoxLayout();
    hA0->addWidget(mkHeader(frame_hospitalInfo, tr("TR_DOTMScreen2")));
    hA0->addWidget(mkSep(frame_hospitalInfo), 1);
    vA->addLayout(hA0);
    QHBoxLayout *hA1 = new QHBoxLayout(); hA1->setContentsMargins(20, 0, 0, 0);
    hA1->addWidget(addChk(frame_hospitalInfo, "chb_hospitalName", tr("TR_HName")));
    hA1->addStretch(1);
    QLineEdit *edHosp = new QLineEdit(frame_hospitalInfo);
    edHosp->setObjectName(QStringLiteral("lineEdit_hospitalName"));
    hA1->addWidget(edHosp);
    vA->addLayout(hA1);
    QHBoxLayout *hA2 = new QHBoxLayout(); hA2->setContentsMargins(20, 0, 0, 0);
    hA2->addWidget(addChk(frame_hospitalInfo, "chb_hospitalLogo", tr("TR_HLog")));
    QPushButton *pbLogo = new QPushButton(tr("TR_Impt"), frame_hospitalInfo);
    pbLogo->setObjectName(QStringLiteral("pbt_importHospitalLogo"));
    pbLogo->setMinimumWidth(60);
    hA2->addWidget(pbLogo);
    hA2->addStretch(1);
    vA->addLayout(hA2);
    vA->addWidget(addChk(frame_hospitalInfo, "chb_btnInfo", tr("TR_RButtons")));
    root->addWidget(frame_hospitalInfo);

    // === B. Инфо пациента ===
    QFrame *frame_patientInfo = new QFrame(host);
    frame_patientInfo->setObjectName(QStringLiteral("frame_patientInfo"));
    QVBoxLayout *vB = new QVBoxLayout(frame_patientInfo);
    QHBoxLayout *hB0 = new QHBoxLayout();
    hB0->addWidget(mkHeader(frame_patientInfo, tr("TR_PIDD2")));
    hB0->addWidget(mkSep(frame_patientInfo), 1);
    vB->addLayout(hB0);
    QGridLayout *gB = new QGridLayout(); gB->setContentsMargins(20, 0, 0, 0);
    gB->addWidget(addChk(frame_patientInfo, "chb_name", tr("TR_Nme")), 0, 0);
    gB->addWidget(addChk(frame_patientInfo, "chb_gender", tr("TR_Gdr")), 0, 1);
    gB->addWidget(addChk(frame_patientInfo, "chb_age", tr("TR_Age")), 0, 2);
    gB->addWidget(addChk(frame_patientInfo, "chb_DOB", tr("TR_DoB")), 1, 0);
    gB->addWidget(addChk(frame_patientInfo, "chb_applicant", tr("TR_Aplct")), 1, 1);
    gB->addWidget(addChk(frame_patientInfo, "chb_patientID", tr("TR_PID")), 1, 2);
    gB->addWidget(addChk(frame_patientInfo, "chb_checkID", tr("TR_ENo")), 2, 0);
    vB->addLayout(gB);
    QVBoxLayout *vB2 = new QVBoxLayout(); vB2->setContentsMargins(20, 0, 0, 0);
    vB2->addWidget(addChk(frame_patientInfo, "chb_custom1", tr("TR_CField1")));
    vB2->addWidget(addChk(frame_patientInfo, "chb_custom2", tr("TR_CField2")));
    vB->addLayout(vB2);
    root->addWidget(frame_patientInfo);

    // === C. Управление учётками ===
    QHBoxLayout *hC0 = new QHBoxLayout();
    hC0->addWidget(mkHeader(host, tr("TR_UManagement")));
    hC0->addWidget(mkSep(host), 1);
    root->addLayout(hC0);
    QHBoxLayout *hC1 = new QHBoxLayout(); hC1->setContentsMargins(20, 0, 0, 0);
    QLabel *lLog = new QLabel(tr("TR_AtLogin"), host); lLog->setObjectName(QStringLiteral("label_logtype"));
    hC1->addWidget(lLog);
    hC1->addWidget(addRadio(host, "radioButton_account", tr("TR_On")));
    hC1->addWidget(addRadio(host, "radioButton_auto", tr("TR_Clse")));
    hC1->addStretch(1);
    root->addLayout(hC1);
    QHBoxLayout *hC2 = new QHBoxLayout(); hC2->setContentsMargins(20, 0, 0, 0);
    QLabel *lFL = new QLabel(tr("TR_FLogout"), host); lFL->setObjectName(QStringLiteral("label_8"));
    hC2->addWidget(lFL);
    hC2->addWidget(addRadio(host, "radio_open", tr("TR_On")));
    hC2->addWidget(addRadio(host, "radio_close", tr("TR_Clse")));
    hC2->addStretch(1);
    root->addLayout(hC2);
    QHBoxLayout *hC3 = new QHBoxLayout(); hC3->setContentsMargins(20, 0, 0, 0);
    QLabel *lDur = new QLabel(tr("TR_FLDuration"), host); lDur->setObjectName(QStringLiteral("label_9"));
    hC3->addWidget(lDur);
    QSpinBox *spTime = new QSpinBox(host);
    spTime->setObjectName(QStringLiteral("spinBox_logout_time"));
    spTime->setRange(1, 9999);
    spTime->setValue(30);   // реф. дефолт
    hC3->addWidget(spTime);
    hC3->addWidget(new QLabel(tr("TR_Mnt"), host));
    hC3->addStretch(1);
    root->addLayout(hC3);

    root->addStretch(1);

    // === D. Кнопки ===
    QHBoxLayout *hD = new QHBoxLayout();
    hD->setObjectName(QStringLiteral("horizontalLayout_7"));
    auto mkBtn = [&](const char *name, const QString &text) {
        QPushButton *b = new QPushButton(host);
        b->setObjectName(QString::fromLatin1(name));
        b->setText(text);
        b->setMinimumWidth(100);
        hD->addWidget(b);
        return b;
    };
    hD->addStretch(1);
    mkBtn("pbt_defalut", tr("TR_Dflt"));   // реф. опечатка objectName сохранена
    mkBtn("pbt_save", tr("TR_Sve"));
    QPushButton *pbExit = mkBtn("pbt_exit", tr("TR_Ext"));
    connect(pbExit, &QPushButton::clicked, this, &QWidget::close);   // реф. OnExit()→close
    hD->addStretch(1);
    root->addLayout(hD);
}
