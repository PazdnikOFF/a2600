#include "ReportConfigDlg.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

ReportConfigDlg::ReportConfigDlg(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x4f2da8: KDialog(modal=false) → new KReportEditUIConfig → setupUi →
    // setStyleSheet(widget_center + label_is_use) → SetTitle(TR_Set). InitWidget/InitConnect
    // вызываются извне (Create-паттерн).
    setupUi();
    SetTitle(tr("TR_Set"));
}

void ReportConfigDlg::setupUi()
{
    setObjectName(QStringLiteral("ReportConfigDlg"));
    resize(1085, 757);

    QWidget *host = ContentArea();
    QVBoxLayout *root = new QVBoxLayout(host);
    root->setContentsMargins(50, 60, 50, 18);
    root->setSpacing(0);

    // === Верхний ряд: тип обследования ===
    QHBoxLayout *hTop = new QHBoxLayout();
    hTop->setContentsMargins(6, 12, 6, 12);
    hTop->addWidget(new QLabel(tr("TR_Tpe"), host));
    QComboBox *cmbType = new QComboBox(host);
    cmbType->setObjectName(QStringLiteral("comboBox_exam_type"));
    cmbType->addItem(tr("TR_Exm") + QStringLiteral(" 1"));   // реф. REPORT_TYPE_1/2/THERAPY
    cmbType->addItem(tr("TR_Exm") + QStringLiteral(" 2"));
    cmbType->addItem(tr("TR_OMode3"));
    hTop->addWidget(cmbType);
    QLabel *lblUse = new QLabel(tr("TR_Used"), host);
    lblUse->setObjectName(QStringLiteral("label_is_use"));
    lblUse->setStyleSheet(QStringLiteral("QLabel{color:cyan;}"));
    hTop->addWidget(lblUse);
    hTop->addStretch(1);
    root->addLayout(hTop);

    // === Центральная панель ===
    QWidget *center = new QWidget(host);
    center->setObjectName(QStringLiteral("widget_center"));
    center->setStyleSheet(QStringLiteral(
        "QWidget#widget_center{background-color:rgb(21,21,21);border-radius:10px;}"));
    QVBoxLayout *v = new QVBoxLayout(center);

    auto fieldCell = [&](QWidget *p, const char *cbName, const QString &labelText) {
        QHBoxLayout *h = new QHBoxLayout();
        QCheckBox *cb = new QCheckBox(p); cb->setObjectName(QString::fromLatin1(cbName));
        h->addWidget(cb);
        h->addWidget(new QLabel(labelText, p));
        h->addStretch(1);
        return h;
    };

    // Секция 1: инфо пациента.
    v->addWidget(new QLabel(tr("TR_PInfo"), center));
    QGridLayout *g1 = new QGridLayout(); g1->setContentsMargins(6, 0, 6, 0);
    struct Cell { int r, c; const char *cb; const char *tr; };
    const Cell s1[] = {
        {0,0,"checkBox_exam_id","TR_ENo"}, {0,1,"checkBox_exam_date","TR_EmDate"},
        {0,2,"checkBox_endoscope","TR_EInfo"}, {0,3,"checkBox_status","TR_Stts"},
        {1,0,"checkBox_name","TR_Nme"}, {1,1,"checkBox_gender","TR_Gdr"},
        {1,2,"checkBox_age","TR_Age"}, {1,3,"checkBox_patient_id","TR_PID"},
        {2,0,"checkBox_applicant","TR_Aplct"}, {2,1,"checkBox_birthday","TR_DoB"},
        {2,2,"checkBox_telphone","TR_Tel"}, {2,3,"checkBox_bed_num","TR_BNo"},
        {3,0,"checkBox_base_info_custom_field1","TR_CField1"},
        {3,1,"checkBox_base_info_custom_field2","TR_CField2"}};
    for (const Cell &c : s1) g1->addLayout(fieldCell(center, c.cb, tr(c.tr)), c.r, c.c);
    v->addLayout(g1);
    v->addSpacing(10);

    // Секция 2: диагноз.
    v->addWidget(new QLabel(tr("TR_Dgnse"), center));
    QGridLayout *g2 = new QGridLayout(); g2->setContentsMargins(6, 0, 6, 0);
    const Cell s2[] = {
        {0,0,"checkBox_exam_finding","TR_VOExam"}, {0,1,"checkBox_diagnose","TR_EConclusion"},
        {0,2,"checkBox_disease_name","TR_DName"}, {0,3,"checkBox_surgical_method","TR_OMode3"},
        {1,0,"checkBox_surgery_finding","TR_IFindings"}, {1,1,"checkBox_suggest","TR_Sgstn"}};
    for (const Cell &c : s2) g2->addLayout(fieldCell(center, c.cb, tr(c.tr)), c.r, c.c);
    v->addLayout(g2);

    // Две польз-строки с lineEdit.
    auto customRow = [&](const char *cbName, const char *editName) {
        QHBoxLayout *h = new QHBoxLayout(); h->setContentsMargins(12, 0, 18, 0);
        QCheckBox *cb = new QCheckBox(center); cb->setObjectName(QString::fromLatin1(cbName));
        h->addWidget(cb);
        h->addWidget(new QLabel(tr("TR_CField1"), center));
        QLineEdit *e = new QLineEdit(center); e->setObjectName(QString::fromLatin1(editName));
        h->addWidget(e, 1);
        v->addLayout(h);
    };
    customRow("checkBox_conclusion_custom_field1", "lineEdit_conclusion_custom_field1");
    customRow("checkBox_conclusion_custom_field2", "lineEdit_conclusion_custom_field2");

    // Секция 3: прочее.
    QGridLayout *g3 = new QGridLayout(); g3->setContentsMargins(4, 0, 6, 0);
    const Cell s3[] = {
        {0,0,"checkBox_biopsy","TR_BSite"}, {0,1,"checkBox_hp","TR_HP"},
        {0,2,"checkBox_assistant_doctor","TR_ADoctor"}, {0,3,"checkBox_doctor","TR_Dctr"}};
    for (const Cell &c : s3) g3->addLayout(fieldCell(center, c.cb, tr(c.tr)), c.r, c.c);
    v->addLayout(g3);
    v->addWidget(new QLabel(tr("TR_VITREWChecked"), center));   // реф. label_31 подсказка

    root->addWidget(center);

    // === Нижний ряд Save/Cancel ===
    QHBoxLayout *hBot = new QHBoxLayout();
    hBot->setContentsMargins(0, 18, 0, 0);
    hBot->addStretch(1);
    QPushButton *btnSave = new QPushButton(tr("TR_Sve"), host);
    btnSave->setObjectName(QStringLiteral("pushButton_save")); btnSave->setMinimumWidth(150);
    QPushButton *btnCancel = new QPushButton(tr("TR_Ccl"), host);
    btnCancel->setObjectName(QStringLiteral("pushButton_cancel")); btnCancel->setMinimumWidth(150);
    hBot->addWidget(btnSave);
    hBot->addWidget(btnCancel);
    hBot->addStretch(1);
    root->addLayout(hBot);

    connect(btnSave, &QPushButton::clicked, this, &QWidget::close);     // реф. OnBtnSave (config write) → close
    connect(btnCancel, &QPushButton::clicked, this, &QWidget::close);   // реф. OnBtnCancel→close
    // Combo-change/checkbox-dirty → KReportEditUIConfig (config) — заглушка.
}
