#include "KColdlightAdjust.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

KColdlightAdjust::KColdlightAdjust(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x747f68: KDialog(modal=false) → setupUi → SetKStyle(2) →
    // title TR_LSConfiguration → QTimer→RefreahData (device) → InitWidget/AllConnect/
    // LoadAutomaticDimmerParam (device).
    setupUi();
    SetKStyle(KDLG_W460);              // реф. SetKStyle(2)
    SetTitle(tr("TR_LSConfiguration"));
}

void KColdlightAdjust::setupUi()
{
    setObjectName(QStringLiteral("KColdlightAdjust"));
    resize(450, 1161);

    QWidget *host = ContentArea();
    QVBoxLayout *root = new QVBoxLayout(host);   // реф. gridLayout_3 (1 колонка)
    root->setContentsMargins(9, 40, 9, 9);       // реф. top=40

    auto dspin = [](QWidget *p, const char *name, double lo, double hi) {
        QDoubleSpinBox *s = new QDoubleSpinBox(p);
        s->setObjectName(QString::fromLatin1(name));
        s->setDecimals(3); s->setSingleStep(0.001);   // реф. dec3, step 0.001
        s->setRange(lo, hi);
        return s;
    };
    auto ispin = [](QWidget *p, const char *name, int lo, int hi) {
        QSpinBox *s = new QSpinBox(p);
        s->setObjectName(QString::fromLatin1(name));
        s->setRange(lo, hi);
        return s;
    };

    // ===================== (1) groupBox_2: время работы лампы =====================
    QGroupBox *grpLamp = new QGroupBox(tr("TR_LLTest"), host);
    grpLamp->setObjectName(QStringLiteral("groupBox_2"));
    grpLamp->setMaximumHeight(120);
    QGridLayout *gL = new QGridLayout(grpLamp);
    gL->setContentsMargins(9, 10, 9, 9);
    gL->addWidget(new QLabel(tr("TR_UTime:"), grpLamp), 0, 0);
    QLabel *lblUse = new QLabel(grpLamp); lblUse->setObjectName(QStringLiteral("label_use_time"));
    gL->addWidget(lblUse, 0, 1, 1, 3);   // device
    QSpinBox *spHour = ispin(grpLamp, "spin_hour", 0, 50000); spHour->setSuffix(QStringLiteral("h"));
    spHour->setMinimumWidth(80);
    gL->addWidget(spHour, 1, 1);
    QSpinBox *spMin = ispin(grpLamp, "spin_minute", 0, 59); spMin->setSuffix(QStringLiteral("min"));
    gL->addWidget(spMin, 1, 2);
    QPushButton *btnSaveTime = new QPushButton(tr("TR_Sve"), grpLamp);
    btnSaveTime->setObjectName(QStringLiteral("btn_save_use_time")); btnSaveTime->setMinimumWidth(50);
    gL->addWidget(btnSaveTime, 1, 3);   // реф. SaveLightUseTime — device
    gL->addWidget(new QLabel(QString::fromUtf8("总使用时间："), grpLamp), 2, 0);
    QLabel *lblTotal = new QLabel(grpLamp); lblTotal->setObjectName(QStringLiteral("label_total_time"));
    gL->addWidget(lblTotal, 2, 1, 1, 3);   // device
    root->addWidget(grpLamp);

    // ===================== (2) groupBox_automatic_dimmer_param =====================
    QGroupBox *grpD = new QGroupBox(QString::fromUtf8("自动调光参数"), host);
    grpD->setObjectName(QStringLiteral("groupBox_automatic_dimmer_param"));
    QVBoxLayout *vD = new QVBoxLayout(grpD);
    // Инфо-строки (device-метки).
    auto infoRow = [&](const QString &cap, const char *valName) {
        QHBoxLayout *h = new QHBoxLayout();
        h->addWidget(new QLabel(cap, grpD));
        QLabel *v = new QLabel(grpD); v->setObjectName(QString::fromLatin1(valName));
        h->addWidget(v); h->addStretch(1);
        vD->addLayout(h);
    };
    infoRow(QString::fromUtf8("光源版本："), "label_coldlight_version");
    infoRow(QString::fromUtf8("镜体类型："), "label_endo_model");
    infoRow(QString::fromUtf8("光源模式："), "label_light_mode");
    // Заголовок Up/Down.
    {
        QHBoxLayout *h = new QHBoxLayout();
        h->addStretch(1);
        h->addWidget(new QLabel(QStringLiteral("Up"), grpD));
        h->addWidget(new QLabel(QStringLiteral("Down"), grpD));
        vD->addLayout(h);
    }
    // PID-строки Up/Down (double, max 10 если не указано иначе).
    auto pidRow = [&](const QString &cap, const char *up, const char *down, double maxDown) {
        QHBoxLayout *h = new QHBoxLayout();
        h->addWidget(new QLabel(cap, grpD));
        h->addWidget(dspin(grpD, up, 0.0, 10.0));
        h->addWidget(dspin(grpD, down, 0.0, maxDown));
        vD->addLayout(h);
    };
    pidRow(QStringLiteral("pAgc:"), "spin_pagc_up", "spin_pagc_down", 10.0);
    pidRow(QStringLiteral("iAgc:"), "spin_iagc_up", "spin_iagc_down", 10.0);
    pidRow(QStringLiteral("dAgc:"), "spin_dagc_up", "spin_dagc_down", 10.0);
    pidRow(QStringLiteral("pAlc:"), "spin_palc_up", "spin_palc_down", 10.0);
    pidRow(QStringLiteral("iAlc:"), "spin_ialc_up", "spin_ialc_down", 99.99);   // реф. down без max
    pidRow(QStringLiteral("dAlc:"), "spin_dalc_up", "spin_dalc_down", 10.0);
    pidRow(QStringLiteral("pAec:"), "spin_paec_up", "spin_paec_down", 10.0);
    pidRow(QStringLiteral("iAec:"), "spin_iaec_up", "spin_iaec_down", 10.0);
    pidRow(QStringLiteral("dAec:"), "spin_daec_up", "spin_daec_down", 10.0);
    // Int-параметры.
    auto intRow = [&](const QString &cap, const char *name, int lo, int hi) {
        QHBoxLayout *h = new QHBoxLayout();
        h->addWidget(new QLabel(cap, grpD));
        h->addWidget(ispin(grpD, name, lo, hi)); h->addStretch(1);
        vD->addLayout(h);
    };
    intRow(QStringLiteral("agc max:"), "spin_agc_max", -6, 27);
    intRow(QStringLiteral("agc min:"), "spin_agc_min", -6, 18);
    intRow(QStringLiteral("threshold:"), "spin_threshold_value", 0, 255);
    // delt_a/delt_r и delt_b/delt_p парами.
    {
        QHBoxLayout *h = new QHBoxLayout();
        h->addWidget(new QLabel(QStringLiteral("delt_a:"), grpD));
        h->addWidget(dspin(grpD, "spin_delt_a", -5.0, 0.0));   // реф. min=-5 max=0
        h->addWidget(new QLabel(QStringLiteral("delt_r:"), grpD));
        h->addWidget(dspin(grpD, "spin_delt_r", 0.0, 10.0));
        vD->addLayout(h);
    }
    {
        QHBoxLayout *h = new QHBoxLayout();
        h->addWidget(new QLabel(QStringLiteral("delt_b:"), grpD));
        h->addWidget(dspin(grpD, "spin_delt_b", 0.0, 10.0));
        h->addWidget(new QLabel(QStringLiteral("delt_p:"), grpD));
        h->addWidget(dspin(grpD, "spin_delt_p", 0.0, 10.0));
        vD->addLayout(h);
    }
    {
        QHBoxLayout *h = new QHBoxLayout();
        h->addWidget(new QLabel(QStringLiteral("overTh:"), grpD));
        h->addWidget(dspin(grpD, "spin_overTh", 0.0, 1.0));   // реф. max=1
        h->addWidget(new QLabel(QStringLiteral("pwmBase"), grpD));
        h->addWidget(ispin(grpD, "spin_pwmbase", 0, 1000));
        vD->addLayout(h);
    }
    // Самотест диммера.
    {
        QHBoxLayout *h = new QHBoxLayout();
        QCheckBox *cbStart = new QCheckBox(QStringLiteral("start "), grpD);
        cbStart->setObjectName(QStringLiteral("checkBox_startTest"));
        h->addWidget(cbStart);
        h->addWidget(new QLabel(QStringLiteral("type"), grpD));
        QComboBox *cmb = new QComboBox(grpD); cmb->setObjectName(QStringLiteral("comboBox_dimTyep"));
        cmb->addItem(QStringLiteral("AGC"), 0); cmb->addItem(QStringLiteral("AEC"), 1);   // статично
        h->addWidget(cmb);
        h->addWidget(new QLabel(QStringLiteral("  flickTh"), grpD));
        h->addWidget(ispin(grpD, "spinBox_flickerTh", 0, 5000));
        vD->addLayout(h);
    }
    {
        QHBoxLayout *h = new QHBoxLayout();
        QCheckBox *cbRnd = new QCheckBox(QStringLiteral("random"), grpD);
        cbRnd->setObjectName(QStringLiteral("checkBox_useRandom"));
        h->addWidget(cbRnd);
        h->addWidget(new QLabel(QStringLiteral("range"), grpD));
        h->addWidget(ispin(grpD, "spinBox_randomRange", 0, 1000));
        h->addWidget(new QLabel(QStringLiteral("fixStep"), grpD));
        h->addWidget(ispin(grpD, "spinBox_fixStep", 0, 1000));
        vD->addLayout(h);
    }
    root->addWidget(grpD);

    // ===================== (3) groupBox TR_AIris: замеры автоапертуры =====================
    QGroupBox *grpIris = new QGroupBox(tr("TR_AIris"), host);
    grpIris->setObjectName(QStringLiteral("groupBox"));
    QGridLayout *gI = new QGridLayout(grpIris);
    auto irisRow = [&](int r, const QString &cap, const char *valName) {
        gI->addWidget(new QLabel(cap, grpIris), r, 0);
        QLabel *v = new QLabel(grpIris); v->setObjectName(QString::fromLatin1(valName));
        gI->addWidget(v, r, 1);   // device
    };
    irisRow(0, QString::fromUtf8("测光值:"), "label_iris");
    irisRow(1, QString::fromUtf8("曝光占比："), "label_ratio");
    irisRow(2, QString::fromUtf8("曝光占比2："), "label_ratio2");
    irisRow(3, QString::fromUtf8("目标亮度："), "label_targetvalue");
    irisRow(4, QString::fromUtf8("AEC："), "label_aec");
    irisRow(5, QString::fromUtf8("AGC："), "label_agc");
    irisRow(6, QString::fromUtf8("ALC："), "label_alc");
    root->addWidget(grpIris);

    // ===================== (4) frame: кнопки =====================
    QFrame *frame = new QFrame(host);
    frame->setObjectName(QStringLiteral("frame"));
    QHBoxLayout *hB = new QHBoxLayout(frame);
    QPushButton *btnSaveParams = new QPushButton(tr("TR_Sve"), frame);
    btnSaveParams->setObjectName(QStringLiteral("btn_save_params"));   // реф. SaveAutomaticDimmerParam — device
    QPushButton *btnExit = new QPushButton(tr("TR_Ext"), frame);
    btnExit->setObjectName(QStringLiteral("btn_exit"));
    btnExit->setFixedWidth(120);   // реф. min=max 120
    hB->addWidget(btnSaveParams);
    hB->addStretch(1);
    hB->addWidget(btnExit);
    connect(btnExit, &QPushButton::clicked, this, &QWidget::close);   // реф. btn_exit→close
    root->addWidget(frame);
}
