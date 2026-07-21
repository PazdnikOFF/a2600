#include "KAlgParamAjustDlg.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

#include <cmath>

namespace {
QString U(const char *s) { return QString::fromUtf8(s); }
} // namespace

KAlgParamAjustDlg::KAlgParamAjustDlg(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x5c9a80: KDialog(modal=false) → setupUi → SetKStyle(2) → title 算法参数调试 →
    // checkBox_all=Checked → InitStatus (device) → InitConnections → 2 QTimer (device-поллеры).
    setupUi();
    SetKStyle(KDLG_W460);              // реф. SetKStyle(2)
    SetTitle(U("算法参数调试"));
}

void KAlgParamAjustDlg::setupUi()
{
    setObjectName(QStringLiteral("KAlgParamAjustDlg"));
    resize(330, 900);

    QWidget *host = ContentArea();
    QVBoxLayout *root = new QVBoxLayout(host);
    root->setContentsMargins(9, 30, 9, 9);

    QStackedWidget *stack = new QStackedWidget(host);
    stack->setObjectName(QStringLiteral("stackedWidget"));

    auto chk = [](QWidget *p, const char *name, const QString &text) {
        QCheckBox *c = new QCheckBox(text, p);
        c->setObjectName(QString::fromLatin1(name));
        return c;
    };
    auto hexSpin = [](QWidget *p, const char *name) {
        QSpinBox *s = new QSpinBox(p);
        s->setObjectName(QString::fromLatin1(name));
        s->setMaximum(0xffff);
        s->setDisplayIntegerBase(16);   // реф. hex-отображение
        return s;
    };
    auto saveBtn = [](QWidget *p, const char *name, const QString &text) {
        QPushButton *b = new QPushButton(text, p);
        b->setObjectName(QString::fromLatin1(name));
        return b;
    };
    // Группа: чекбокс «开启» сверху + контент + кнопка сохранения снизу.
    auto grid3x3 = [&](QWidget *p, const char *prefix) {
        QGridLayout *g = new QGridLayout();
        for (int i = 0; i < 9; ++i)
            g->addWidget(hexSpin(p, QStringLiteral("%1_%2").arg(prefix).arg(i + 1).toUtf8().constData()),
                         i / 3, i % 3);
        return g;
    };

    // ===================== Страница 1 =====================
    QWidget *page1 = new QWidget(); page1->setObjectName(QStringLiteral("page"));
    QVBoxLayout *v1 = new QVBoxLayout(page1);

    // SCL (特殊光模式).
    QGroupBox *gScl = new QGroupBox(U("特殊光模式"), page1); gScl->setObjectName(QStringLiteral("groupBox_scl"));
    QVBoxLayout *vScl = new QVBoxLayout(gScl);
    vScl->addWidget(chk(gScl, "checkBox_scl", U("开启")));
    vScl->addLayout(grid3x3(gScl, "spinBox_scl"));
    vScl->addWidget(saveBtn(gScl, "pb_scl_save", U("保存并下发")));
    v1->addWidget(gScl);

    // Мастер-переключатель (算法总开关).
    QGroupBox *gAll = new QGroupBox(U("算法总开关"), page1); gAll->setObjectName(QStringLiteral("groupBox"));
    QVBoxLayout *vAll = new QVBoxLayout(gAll);
    QCheckBox *cbAll = chk(gAll, "checkBox_all", U("开启")); cbAll->setChecked(true);   // реф. ctor Checked
    vAll->addWidget(cbAll);
    v1->addWidget(gAll);

    // CCM.
    QGroupBox *gCcm = new QGroupBox(QStringLiteral("CCM"), page1); gCcm->setObjectName(QStringLiteral("groupBox_ccm"));
    QVBoxLayout *vCcm = new QVBoxLayout(gCcm);
    vCcm->addWidget(chk(gCcm, "checkBox_ccm", U("开启")));
    vCcm->addLayout(grid3x3(gCcm, "spinBox_ccm"));
    vCcm->addWidget(saveBtn(gCcm, "pb_ccm_save", U("保存并下发")));
    v1->addWidget(gCcm);

    // AWB (白平衡).
    QGroupBox *gAwb = new QGroupBox(U("白平衡"), page1); gAwb->setObjectName(QStringLiteral("groupBox_awb"));
    QVBoxLayout *vAwb = new QVBoxLayout(gAwb);
    vAwb->addWidget(chk(gAwb, "checkBox_awb", U("开启")));
    QGridLayout *gAwbGrid = new QGridLayout();
    gAwbGrid->addWidget(new QLabel(QStringLiteral("T"), gAwb), 0, 0);
    gAwbGrid->addWidget(new QLabel(QStringLiteral("para"), gAwb), 0, 2);
    gAwbGrid->addWidget(new QLabel(QStringLiteral("cfg"), gAwb), 0, 4);
    gAwbGrid->addWidget(new QLabel(QStringLiteral("up"), gAwb), 1, 0);
    gAwbGrid->addWidget(hexSpin(gAwb, "spinBox_awb_up"), 1, 1);
    gAwbGrid->addWidget(new QLabel(QStringLiteral("r"), gAwb), 1, 2);
    gAwbGrid->addWidget(hexSpin(gAwb, "spinBox_awb_r"), 1, 3);
    gAwbGrid->addWidget(new QLabel(QStringLiteral("wbr"), gAwb), 1, 4);
    gAwbGrid->addWidget(hexSpin(gAwb, "spinBox_awb_wbr"), 1, 5);
    gAwbGrid->addWidget(new QLabel(QStringLiteral("dw"), gAwb), 2, 0);
    gAwbGrid->addWidget(hexSpin(gAwb, "spinBox_awb_dw"), 2, 1);
    gAwbGrid->addWidget(new QLabel(QStringLiteral("b"), gAwb), 2, 2);
    gAwbGrid->addWidget(hexSpin(gAwb, "spinBox_awb_b"), 2, 3);
    gAwbGrid->addWidget(new QLabel(QStringLiteral("wbb"), gAwb), 2, 4);
    gAwbGrid->addWidget(hexSpin(gAwb, "spinBox_awb_wbb"), 2, 5);
    vAwb->addLayout(gAwbGrid);
    vAwb->addWidget(saveBtn(gAwb, "pb_awb_save", U("保存")));
    v1->addWidget(gAwb);
    v1->addStretch(1);
    stack->addWidget(page1);

    // ===================== Страница 2 =====================
    QWidget *page2 = new QWidget(); page2->setObjectName(QStringLiteral("page_2"));
    QVBoxLayout *v2 = new QVBoxLayout(page2);

    // Knee.
    QGroupBox *gKnee = new QGroupBox(QStringLiteral("Knee"), page2); gKnee->setObjectName(QStringLiteral("groupBox_graphEnh"));
    QGridLayout *gK = new QGridLayout(gKnee);
    gK->addWidget(chk(gKnee, "checkBox_knee", U("开启")), 0, 0, 1, 2);
    gK->addWidget(new QLabel(QStringLiteral("Knee"), gKnee), 1, 0); gK->addWidget(hexSpin(gKnee, "spinBox_knee"), 1, 1);
    gK->addWidget(new QLabel(QStringLiteral("slp"), gKnee), 2, 0);
    QDoubleSpinBox *dSlp = new QDoubleSpinBox(gKnee); dSlp->setObjectName(QStringLiteral("doubleSpinBox_slp")); gK->addWidget(dSlp, 2, 1);
    gK->addWidget(new QLabel(QStringLiteral("slp2"), gKnee), 3, 0);
    QDoubleSpinBox *dSlp2 = new QDoubleSpinBox(gKnee); dSlp2->setObjectName(QStringLiteral("doubleSpinBox_slp2")); gK->addWidget(dSlp2, 3, 1);
    gK->addWidget(new QLabel(QStringLiteral("inputmax"), gKnee), 4, 0); gK->addWidget(hexSpin(gKnee, "spinBox_knee_inputmax"), 4, 1);
    gK->addWidget(new QLabel(QStringLiteral("outputmax"), gKnee), 5, 0); gK->addWidget(hexSpin(gKnee, "spinBox_knee_outputmax"), 5, 1);
    gK->addWidget(saveBtn(gKnee, "pb_knee_save", U("保存并下发")), 6, 0, 1, 2);
    v2->addWidget(gKnee);
    // slp valueChanged → refresh slp2 range (реф. Slot_refresh_slp2_range) — чистый UI.
    connect(dSlp, QOverload<double>::of(&QDoubleSpinBox::valueChanged), dSlp2,
            [dSlp2](double v) { dSlp2->setMaximum(v > 0 ? v : 99.99); });

    // Gamma.
    QGroupBox *gGma = new QGroupBox(QStringLiteral("Gamma"), page2); gGma->setObjectName(QStringLiteral("groupBox_gmma"));
    QGridLayout *gG = new QGridLayout(gGma);
    gG->addWidget(chk(gGma, "checkBox_gamma", U("开启")), 0, 0, 1, 2);
    gG->addWidget(new QLabel(QStringLiteral("bp"), gGma), 1, 0);
    QDoubleSpinBox *dBp = new QDoubleSpinBox(gGma); dBp->setObjectName(QStringLiteral("doubleSpinBox_bp")); gG->addWidget(dBp, 1, 1);
    gG->addWidget(new QLabel(QStringLiteral("gamma"), gGma), 2, 0);
    QDoubleSpinBox *dGamma = new QDoubleSpinBox(gGma); dGamma->setObjectName(QStringLiteral("doubleSpinBox_gamma")); gG->addWidget(dGamma, 2, 1);
    gG->addWidget(new QLabel(QStringLiteral("inputmax"), gGma), 3, 0); gG->addWidget(hexSpin(gGma, "spinBox_gamma_inputmax"), 3, 1);
    gG->addWidget(saveBtn(gGma, "pb_gamma_save", U("保存并下发")), 4, 0, 1, 2);
    v2->addWidget(gGma);

    // Denoise (图像降噪) — level combo.
    auto levelGroup = [&](const char *gName, const QString &title, const char *cbName,
                          const char *cmbName, const char *saveName) {
        QGroupBox *gb = new QGroupBox(title, page2); gb->setObjectName(QString::fromLatin1(gName));
        QVBoxLayout *v = new QVBoxLayout(gb);
        v->addWidget(chk(gb, cbName, U("开启")));
        QHBoxLayout *h = new QHBoxLayout();
        h->addWidget(new QLabel(U("等级"), gb));
        QComboBox *cmb = new QComboBox(gb); cmb->setObjectName(QString::fromLatin1(cmbName));
        cmb->addItem(U("低")); cmb->addItem(U("中")); cmb->addItem(U("高"));   // реф. статично
        h->addWidget(cmb); h->addStretch(1);
        v->addLayout(h);
        v->addWidget(saveBtn(gb, saveName, U("保存并下发")));
        return gb;
    };
    v2->addWidget(levelGroup("groupBox_denoise", U("图像降噪"), "checkBox_denoise",
                             "comboBox_denoise_level", "pb_denoise_save"));
    v2->addWidget(levelGroup("groupBox_2", U("亮度均衡"), "checkBox_brightBalence",
                             "comboBox_brightBalence_level", "pb_brightBalence_save"));

    // Ряд чекбоксов фич.
    QHBoxLayout *hFeat1 = new QHBoxLayout();
    hFeat1->addWidget(chk(page2, "checkBox_sencorLut", QStringLiteral("SensorLUT")));
    hFeat1->addWidget(chk(page2, "checkBox_colorEnh", U("色彩增强")));
    hFeat1->addWidget(chk(page2, "checkBox_graphEnh", U("图像增强")));
    v2->addLayout(hFeat1);
    QHBoxLayout *hFeat2 = new QHBoxLayout();
    hFeat2->addWidget(chk(page2, "checkBox_dehaze", U("电子去烟")));
    hFeat2->addWidget(chk(page2, "checkBox_hdr", U("宽动态范围")));
    hFeat2->addStretch(1);
    v2->addLayout(hFeat2);

    // Exit / next.
    QHBoxLayout *hNav = new QHBoxLayout();
    QPushButton *pbExit = new QPushButton(U("退出"), page2); pbExit->setObjectName(QStringLiteral("pb_exit"));
    QPushButton *pbNext = new QPushButton(QStringLiteral(">"), page2); pbNext->setObjectName(QStringLiteral("pb_nextPage"));
    hNav->addWidget(pbExit); hNav->addStretch(1); hNav->addWidget(pbNext);
    v2->addLayout(hNav);

    // Конвертер定点浮点转换.
    QGroupBox *gConv = new QGroupBox(U("定点浮点转换"), page2); gConv->setObjectName(QStringLiteral("groupBox_3"));
    QGridLayout *gC = new QGridLayout(gConv);
    gC->addWidget(new QLabel(U("整数位宽："), gConv), 0, 0);
    QSpinBox *spInt = new QSpinBox(gConv); spInt->setObjectName(QStringLiteral("spinBox_integerWidth")); gC->addWidget(spInt, 0, 1);
    gC->addWidget(new QLabel(U("小数位宽："), gConv), 1, 0);
    QSpinBox *spFrac = new QSpinBox(gConv); spFrac->setObjectName(QStringLiteral("spinBox_fractionalWidth"));
    spFrac->setRange(0, 31); spFrac->setValue(8); gC->addWidget(spFrac, 1, 1);
    gC->addWidget(new QLabel(U("定点："), gConv), 2, 0);
    gC->addWidget(new QLabel(QStringLiteral("0x"), gConv), 2, 1);
    QSpinBox *spFix = new QSpinBox(gConv); spFix->setObjectName(QStringLiteral("spinBox_fixNum"));
    spFix->setMaximum(0x7fffffff); spFix->setDisplayIntegerBase(16); gC->addWidget(spFix, 2, 2);
    QPushButton *btnI2F = new QPushButton(U("转换"), gConv); btnI2F->setObjectName(QStringLiteral("btn_int2float")); gC->addWidget(btnI2F, 2, 3);
    gC->addWidget(new QLabel(U("浮点："), gConv), 3, 0);
    QDoubleSpinBox *dFloat = new QDoubleSpinBox(gConv); dFloat->setObjectName(QStringLiteral("doubleSpinBox_floatNum"));
    dFloat->setDecimals(6); dFloat->setRange(-1e6, 1e6); gC->addWidget(dFloat, 3, 2);
    QPushButton *btnF2I = new QPushButton(U("转换"), gConv); btnF2I->setObjectName(QStringLiteral("btn_float2int")); gC->addWidget(btnF2I, 3, 3);
    v2->addWidget(gConv);
    v2->addStretch(1);
    stack->addWidget(page2);

    // Конвертер fixed↔float — чистая арифметика (реф. Slot_fixnum_to_floatnum/floatnum_to_fixnum).
    connect(btnI2F, &QPushButton::clicked, this, [=]() {
        dFloat->setValue(double(spFix->value()) / std::pow(2.0, spFrac->value()));
    });
    connect(btnF2I, &QPushButton::clicked, this, [=]() {
        spFix->setValue(int(std::lround(dFloat->value() * std::pow(2.0, spFrac->value()))));
    });

    root->addWidget(stack);

    // pb_nextPage переключает страницы (реф. Slot_pb_nextPage) — чистый UI.
    connect(pbNext, &QPushButton::clicked, stack, [stack]() {
        stack->setCurrentIndex(stack->currentIndex() == 0 ? 1 : 0);
    });
    connect(pbExit, &QPushButton::clicked, this, &QWidget::close);   // реф. Slot_pb_exit→close
    // Slot_checkBox_*/Slot_pb_*_save (push в FPGA), InitStatus, QTimer-поллеры — DEVICE.
}
