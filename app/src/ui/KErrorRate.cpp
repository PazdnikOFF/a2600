#include "KErrorRate.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegExpValidator>
#include <QVBoxLayout>
#include <QWidget>

KErrorRate::KErrorRate(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x5e1768: KDialog(modal=false) → setupUi → SetKStyle(2) → title 误码率测试 →
    // InitStatus (валидаторы/дефолты/enable) → InitConnections → QTimer (device-тик).
    setupUi();
    SetKStyle(KDLG_W460);              // реф. SetKStyle(2)
    SetTitle(QString::fromUtf8("误码率测试"));
}

void KErrorRate::setupUi()
{
    setObjectName(QStringLiteral("KErrorRate"));
    resize(284, 728);

    QWidget *host = ContentArea();
    QVBoxLayout *root = new QVBoxLayout(host);
    root->setContentsMargins(9, 30, 9, 9);   // реф. top margin 30

    auto valEdit = [&](const char *name, const QString &def, const QString &rx) {
        QLineEdit *e = new QLineEdit(def, host);
        e->setObjectName(QString::fromLatin1(name));
        e->setValidator(new QRegExpValidator(QRegExp(rx), e));
        return e;
    };
    auto valueLbl = [&](QWidget *p, const char *name, const QString &init) {
        QLabel *l = new QLabel(init, p);
        l->setObjectName(QString::fromLatin1(name));
        return l;   // device-updated
    };
    auto row = [&](const QString &cap, QWidget *val) {
        QHBoxLayout *h = new QHBoxLayout();
        h->addWidget(new QLabel(cap, host));
        h->addWidget(val); h->addStretch(1);
        return h;
    };

    // === Группа «结果» ===
    QGroupBox *grpResult = new QGroupBox(QString::fromUtf8("结果"), host);
    grpResult->setObjectName(QStringLiteral("groupBox"));
    QGridLayout *gR = new QGridLayout(grpResult);
    gR->addWidget(valueLbl(grpResult, "label_result", QString()), 0, 0);   // device
    root->addWidget(grpResult);

    // === Чекбокс «出错后停止调试» ===
    QCheckBox *chk = new QCheckBox(QString::fromUtf8("出错后停止调试"), host);
    chk->setObjectName(QStringLiteral("checkBox"));
    chk->setChecked(false);
    root->addWidget(chk);

    // === Параметры теста ===
    QVBoxLayout *vParam = new QVBoxLayout();
    // Интервал теста (s).
    QHBoxLayout *hInt = new QHBoxLayout();
    hInt->addWidget(new QLabel(QString::fromUtf8("测试间隔(s)："), host));
    hInt->addStretch(1);
    hInt->addWidget(valEdit("lineEdit_testInterval", QStringLiteral("2"),
                            QStringLiteral("[1-9]|[1-5][0-9]|60")));   // 1..60
    hInt->addStretch(1);
    vParam->addLayout(hInt);
    // Общее время теста h/m.
    QHBoxLayout *hTot = new QHBoxLayout();
    hTot->addWidget(new QLabel(QString::fromUtf8("总测试时间 ："), host));
    hTot->addStretch(1);
    hTot->addWidget(valEdit("lineEdit_testTime_h", QStringLiteral("1"),
                            QStringLiteral("0|[1-9][0-9]{0,1}")));   // 0..99
    hTot->addWidget(new QLabel(QStringLiteral("h"), host));
    hTot->addWidget(valEdit("lineEdit_testTime_m", QStringLiteral("0"),
                            QStringLiteral("[0-9]|[1-5][0-9]")));   // 0..59
    hTot->addWidget(new QLabel(QStringLiteral("m"), host));
    vParam->addLayout(hTot);
    vParam->addLayout(row(QString::fromUtf8("已测试时间 ："), valueLbl(host, "label_timePassed", QStringLiteral("00:00"))));
    vParam->addLayout(row(QString::fromUtf8("总测试次数 ："), valueLbl(host, "label_timesSummary", QStringLiteral("0"))));
    vParam->addLayout(row(QString::fromUtf8("总错误次数 ："), valueLbl(host, "label_errorsSummary", QStringLiteral("0"))));
    root->addLayout(vParam);

    // === Группа «阈值» (пороги) ===
    QGroupBox *grpTh = new QGroupBox(QString::fromUtf8("阈值"), host);
    grpTh->setObjectName(QStringLiteral("groupBox_2"));
    QHBoxLayout *hTh = new QHBoxLayout(grpTh);
    hTh->addWidget(new QLabel(QString::fromUtf8("风险 ："), grpTh));
    hTh->addWidget(valEdit("lineEdit_limit_risky", QStringLiteral("1"), QStringLiteral("[1-9][0-9]{0,1}")));
    hTh->addStretch(1);
    hTh->addWidget(new QLabel(QStringLiteral("NG ："), grpTh));
    hTh->addWidget(valEdit("lineEdit_limit_NG", QStringLiteral("1"), QStringLiteral("[1-9][0-9]{0,1}")));
    hTh->addStretch(1);
    root->addWidget(grpTh);

    // === Группа «错误统计» ===
    QGroupBox *grpErr = new QGroupBox(QString::fromUtf8("错误统计"), host);
    grpErr->setObjectName(QStringLiteral("groupBox_3"));
    QVBoxLayout *vErr = new QVBoxLayout(grpErr);
    auto errRow = [&](const QString &cap, const char *valName) {
        QHBoxLayout *h = new QHBoxLayout();
        h->addWidget(new QLabel(cap, grpErr));
        h->addWidget(valueLbl(grpErr, valName, QStringLiteral("0"))); h->addStretch(1);
        vErr->addLayout(h);
    };
    errRow(QString::fromUtf8("寄存器最新值："), "label_registerValue");
    errRow(QStringLiteral("Lane0 Error:"), "label_lane0Error");
    errRow(QStringLiteral("Lane1 Error:"), "label_lane1Error");
    errRow(QString::fromUtf8("CRC Error："), "label_CRC_Error");
    errRow(QString::fromUtf8("ECC Error："), "label_ECC_Error");
    errRow(QString::fromUtf8("SOT Error："), "label_SOT_Error");
    root->addWidget(grpErr);

    root->addStretch(1);

    // === Кнопки (стейт-машина enable — чистый UI) ===
    auto ctlBtn = [&](const char *name, const QString &text, bool enabled) {
        QPushButton *b = new QPushButton(text, host);
        b->setObjectName(QString::fromLatin1(name));
        b->setMinimumSize(120, 30);   // реф. InitStatus
        b->setEnabled(enabled);
        return b;
    };
    QPushButton *btnStart = ctlBtn("pb_start", QString::fromUtf8("开始"), true);
    QPushButton *btnStop = ctlBtn("pb_stop", QString::fromUtf8("停止"), false);
    QPushButton *btnPause = ctlBtn("pb_pause", QString::fromUtf8("暂停"), false);
    QPushButton *btnContinue = ctlBtn("pb_continue", QString::fromUtf8("继续"), false);
    QHBoxLayout *h13 = new QHBoxLayout();
    h13->addStretch(1); h13->addWidget(btnStart); h13->addStretch(1);
    h13->addWidget(btnStop); h13->addStretch(1);
    root->addLayout(h13);
    QHBoxLayout *h14 = new QHBoxLayout();
    h14->addStretch(1); h14->addWidget(btnPause); h14->addStretch(1);
    h14->addWidget(btnContinue); h14->addStretch(1);
    root->addLayout(h14);

    // Реф. SlotStart/Stop/Pause/Continue — BER-тест (device); переходы enable — чистый UI.
    connect(btnStart, &QPushButton::clicked, this, [=]() {
        btnStart->setEnabled(false); btnStop->setEnabled(true);
        btnPause->setEnabled(true); btnContinue->setEnabled(false);
    });
    connect(btnStop, &QPushButton::clicked, this, [=]() {
        btnStart->setEnabled(true); btnStop->setEnabled(false);
        btnPause->setEnabled(false); btnContinue->setEnabled(false);
    });
    connect(btnPause, &QPushButton::clicked, this, [=]() {
        btnPause->setEnabled(false); btnContinue->setEnabled(true);
    });
    connect(btnContinue, &QPushButton::clicked, this, [=]() {
        btnContinue->setEnabled(false); btnPause->setEnabled(true);
    });
}
