#include "KPatientListSetupDlg.h"
#include "KMessageBox.h"

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QDateEdit>
#include <QComboBox>
#include <QGroupBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDate>

KPatientListSetupDlg::KPatientListSetupDlg(QWidget *parent)
    : KDialog(parent, false)
{
    // Реф. ctor @0x7dc730: setupUi → SetKStyle(7) → title → LoadConfigData(device) → Initconnect.
    SetKStyle(KDLG_W1024);
    SetTitle(tr("TR_PLSettings"));
    buildUi();
}

void KPatientListSetupDlg::buildUi()
{
    setObjectName(QStringLiteral("KPatientListSetupDlg"));
    QWidget *content = ContentArea();
    QVBoxLayout *outer = new QVBoxLayout(content);

    auto mkChk = [&](const char *name, const QString &text) {
        QCheckBox *c = new QCheckBox(text, content);
        c->setObjectName(QString::fromLatin1(name));
        return c;
    };
    auto mkEdit = [&](const char *name) {
        QLineEdit *e = new QLineEdit(content);
        e->setObjectName(QString::fromLatin1(name));
        return e;
    };

    // ── grb_patientlist (9 колонок отображения) ──
    QGroupBox *grbP = new QGroupBox(content);
    grbP->setObjectName(QStringLiteral("grb_patientlist"));
    QVBoxLayout *vP = new QVBoxLayout(grbP);
    QLabel *lblList = new QLabel(tr("TR_LDisplay"), grbP);
    lblList->setObjectName(QStringLiteral("label_list"));
    vP->addWidget(lblList);
    QLabel *lblTip = new QLabel(tr("TR_VITAPAPLSWChecked"), grbP);
    lblTip->setObjectName(QStringLiteral("label_tip"));
    lblTip->setStyleSheet(QStringLiteral("color:rgb(153,153,153);"));
    vP->addWidget(lblTip);

    QGridLayout *gP = new QGridLayout();
    // Польз-поле 1: чекбокс + paired edit заголовка.
    gP->addWidget(mkChk("chb_useritem1", tr("TR_CField1")), 0, 0);
    gP->addWidget(mkEdit("edit_useritem1"), 0, 1);
    gP->addWidget(mkChk("chb_patientid", tr("TR_PID")), 0, 2);
    gP->addWidget(mkChk("chb_useritem2", tr("TR_CField2")), 1, 0);
    gP->addWidget(mkEdit("edit_useritem2"), 1, 1);
    gP->addWidget(mkChk("chb_applytime", tr("TR_ADate")), 1, 2);
    gP->addWidget(mkChk("chb_telephone", tr("TR_Tel")), 2, 0);
    gP->addWidget(mkChk("chb_pregister_number", tr("TR_ANumber")), 2, 1);
    gP->addWidget(mkChk("chb_applicant", tr("TR_Aplct")), 2, 2);
    gP->addWidget(mkChk("chb_bedno", tr("TR_BNo")), 3, 0);
    gP->addWidget(mkChk("chb_dob", tr("TR_DoB")), 3, 1);
    vP->addLayout(gP);
    outer->addWidget(grbP);

    // ── grb_worklist (5 колонок запроса + paired default-инпуты) ──
    QGroupBox *grbW = new QGroupBox(content);
    grbW->setObjectName(QStringLiteral("grb_worklist"));
    QVBoxLayout *vW = new QVBoxLayout(grbW);
    QLabel *lblWl = new QLabel(tr("TR_wQuery"), grbW);
    lblWl->setObjectName(QStringLiteral("label_worklist"));
    vW->addWidget(lblWl);

    QGridLayout *gW = new QGridLayout();
    gW->addWidget(mkChk("chb_worklist_patientid", tr("TR_PID2")), 0, 0);
    gW->addWidget(mkEdit("edit_patientid"), 0, 1);
    gW->addWidget(mkChk("chb_name", tr("TR_Nme:")), 0, 2);
    gW->addWidget(mkEdit("edit_name"), 0, 3);
    gW->addWidget(mkChk("chb_wregister_number", tr("TR_ANumber:")), 1, 0);
    gW->addWidget(mkEdit("edit_register_number"), 1, 1);
    // Устройство обследования: чекбокс + combo (первый пункт TR_Escpy).
    gW->addWidget(mkChk("chb_exam_device", tr("TR_EEquipment:")), 1, 2);
    QComboBox *cmbEquip = new QComboBox(content);
    cmbEquip->setObjectName(QStringLiteral("cmb_equipment"));
    cmbEquip->addItem(tr("TR_Escpy"));
    gW->addWidget(cmbEquip, 1, 3);
    // План-время: чекбокс + диапазон дат.
    gW->addWidget(mkChk("chb_plantime", tr("TR_PTime:")), 2, 0);
    QWidget *wRange = new QWidget(content);
    QHBoxLayout *hR = new QHBoxLayout(wRange); hR->setContentsMargins(0, 0, 0, 0);
    QDateEdit *d1 = new QDateEdit(wRange); d1->setObjectName(QStringLiteral("date_plantime1"));
    d1->setDisplayFormat(QStringLiteral("yyyy-MM-dd")); d1->setDate(QDate::currentDate());
    QDateEdit *d2 = new QDateEdit(wRange); d2->setObjectName(QStringLiteral("date_plantime2"));
    d2->setDisplayFormat(QStringLiteral("yyyy-MM-dd")); d2->setDate(QDate::currentDate());
    hR->addWidget(d1); hR->addWidget(new QLabel(QStringLiteral("--"), wRange)); hR->addWidget(d2);
    gW->addWidget(wRange, 2, 1, 1, 3);
    vW->addLayout(gW);
    outer->addWidget(grbW);

    // ── кнопки ──
    QHBoxLayout *btns = new QHBoxLayout();
    btns->addStretch();
    QPushButton *btnSave = new QPushButton(tr("TR_Sve"), content);
    btnSave->setObjectName(QStringLiteral("btn_save"));
    QPushButton *btnExit = new QPushButton(tr("TR_Ext"), content);
    btnExit->setObjectName(QStringLiteral("btn_exit"));
    btns->addWidget(btnSave); btns->addWidget(btnExit);
    outer->addLayout(btns);

    connect(btnSave, &QPushButton::clicked, this, &KPatientListSetupDlg::SaveSetupData);
    connect(btnExit, &QPushButton::clicked, this, &KPatientListSetupDlg::ExitSetupData);
}

void KPatientListSetupDlg::SaveSetupData()
{
    // Реф. @0x7dc8e8: isChecked→SetIsShow* по каждому чекбоксу + SaveConfig обоих хендлеров +
    // PublishMsg(12004/12008/12007). DEVICE-STUB — только закрыть.
    close();
}

void KPatientListSetupDlg::ExitSetupData()
{
    // Реф. @0x7dd5a8: спросить «сохранить?» (dirty), Yes→SaveSetupData, затем close.
    const int r = KMessageBox::question(this, tr("TR_Wng"), tr("TR_Sve?"),
                                        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (r == QMessageBox::Yes)
        SaveSetupData();
    else
        close();
}
