#include "KCustomEdit.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

namespace {
// Реф. KLineH — горизонтальный сепаратор (QFrame HLine/Sunken).
QFrame *mkLineH(QWidget *p)
{
    QFrame *f = new QFrame(p);
    f->setFrameShape(QFrame::HLine);
    f->setFrameShadow(QFrame::Sunken);
    return f;
}
// Заголовок секции: подпись (жирная) + разделитель (рев. label_* + line_* KLineH).
QHBoxLayout *sectionHeader(QWidget *p, const QString &text)
{
    QHBoxLayout *h = new QHBoxLayout();
    QLabel *l = new QLabel(text, p);
    l->setStyleSheet(QStringLiteral("font-weight:bold;"));
    h->addWidget(l);
    h->addWidget(mkLineH(p), 1);
    return h;
}
QComboBox *mkCombo(QWidget *p, const char *name)
{
    QComboBox *c = new QComboBox(p);
    c->setObjectName(QString::fromLatin1(name));
    return c;   // список статичен (InitWidget), но значения device — оставляем пустым
}
QLabel *mkLbl(QWidget *p, const QString &t, const char *name)
{
    QLabel *l = new QLabel(t, p);
    l->setObjectName(QString::fromLatin1(name));
    return l;
}
} // namespace

KCustomEdit::KCustomEdit(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x633e20: KDialog(modal=true) → setupUi → SetKStyle(2) → title TR_PSet →
    // InitWidget/InitVar → connects → ConnectEditSignal (device) → снимок VideoParamConfig.
    setModal(true);                    // реф. KDialog(this,parent,true)
    setupUi();
    SetKStyle(KDLG_W460);              // реф. SetKStyle(2)
    SetTitle(tr("TR_PSet"));
}

void KCustomEdit::setupUi()
{
    setObjectName(QStringLiteral("KCustomEdit"));
    resize(578, 1080);

    QWidget *host = ContentArea();
    QVBoxLayout *root = new QVBoxLayout(host);
    root->setObjectName(QStringLiteral("gridLayout_15"));

    // ===================== СТРАНИЦА 1 =====================
    m_page1 = new QFrame(host);
    m_page1->setObjectName(QStringLiteral("frame_page1"));
    m_page1->setFrameShape(QFrame::StyledPanel);
    m_page1->setFrameShadow(QFrame::Raised);
    QVBoxLayout *p1 = new QVBoxLayout(m_page1);

    // --- Усиление ---
    p1->addLayout(sectionHeader(m_page1, tr("TR_Enhnce")));
    QGridLayout *gEnh = new QGridLayout();
    gEnh->addWidget(mkLbl(m_page1, tr("TR_IEnh"), "label_ImgEnh"), 0, 1);
    gEnh->addWidget(mkLbl(m_page1, tr("TR_CEnh"), "label_ColorEnh"), 0, 2);
    gEnh->addWidget(mkLbl(m_page1, tr("TR_Tpe"), "label_2"), 1, 0);
    gEnh->addWidget(mkCombo(m_page1, "cmb_ienhtype"), 1, 1);
    gEnh->addWidget(mkLbl(m_page1, QStringLiteral("L1"), "label_Mode1"), 2, 0);
    gEnh->addWidget(mkCombo(m_page1, "cmb_ImgEnh1"), 2, 1);
    gEnh->addWidget(mkCombo(m_page1, "cmb_ColorEnh1"), 2, 2);
    gEnh->addWidget(mkLbl(m_page1, QStringLiteral("L2"), "label_Mode2"), 3, 0);
    gEnh->addWidget(mkCombo(m_page1, "cmb_ImgEnh2"), 3, 1);
    gEnh->addWidget(mkCombo(m_page1, "cmb_ColorEnh2"), 3, 2);
    gEnh->addWidget(mkLbl(m_page1, QStringLiteral("L3"), "label_Mode3"), 4, 0);
    gEnh->addWidget(mkCombo(m_page1, "cmb_ImgEnh3"), 4, 1);
    gEnh->addWidget(mkCombo(m_page1, "cmb_ColorEnh3"), 4, 2);
    p1->addLayout(gEnh);

    // --- Цвет: баланс RGB (radio + спин) ---
    p1->addLayout(sectionHeader(m_page1, tr("TR_Col")));
    QGridLayout *gRgb = new QGridLayout();
    const char *rgbR[3] = {"R", "B", "C"};
    const char *rgbRN[3] = {"radio_R", "radio_B", "radio_C"};
    const char *rgbSN[3] = {"spin_R", "spin_B", "spin_C"};
    for (int i = 0; i < 3; ++i) {
        QRadioButton *rb = new QRadioButton(QString::fromLatin1(rgbR[i]), m_page1);
        rb->setObjectName(QString::fromLatin1(rgbRN[i]));
        gRgb->addWidget(rb, i, 0);
        QSpinBox *sp = new QSpinBox(m_page1);
        sp->setObjectName(QString::fromLatin1(rgbSN[i]));
        gRgb->addWidget(sp, i, 1);   // диапазон/значение — device
    }
    p1->addLayout(gRgb);

    // --- Зум (double spin) ---
    p1->addLayout(sectionHeader(m_page1, tr("TR_Zm1")));
    QGridLayout *gZoom = new QGridLayout();
    const char *zL[3] = {"L1", "L2", "L3"};
    const char *zN[3] = {"spin_zoom1", "spin_zoom2", "spin_zoom3"};
    const char *zLN[3] = {"label_zoom1", "label_zoom2", "label_zoom3"};
    for (int i = 0; i < 3; ++i) {
        gZoom->addWidget(mkLbl(m_page1, QString::fromLatin1(zL[i]), zLN[i]), i, 0);
        QDoubleSpinBox *sp = new QDoubleSpinBox(m_page1);
        sp->setObjectName(QString::fromLatin1(zN[i]));
        sp->setDecimals(1);
        if (i == 0) { sp->setRange(1.0, 1.0); sp->setEnabled(false); }  // реф. read-only базовый зум
        else        { sp->setRange(1.1, 4.0); }                          // реф. L2/L3 1.1–4.0
        gZoom->addWidget(sp, i, 1);
    }
    p1->addLayout(gZoom);

    // --- Кнопки эндоскопа (переключатели 0..3) ---
    p1->addLayout(sectionHeader(m_page1, tr("TR_RButtons")));
    QGridLayout *gSw = new QGridLayout();
    const char *swN[4] = {"cmb_swtich1", "cmb_swtich2", "cmb_swtich3", "cmb_swtich4"};
    const char *swLN[4] = {"label_switch1", "label_switch2", "label_swtich3", "label_swtich4"};
    for (int i = 0; i < 4; ++i) {
        gSw->addWidget(mkLbl(m_page1, QString::number(i), swLN[i]), i, 0);
        gSw->addWidget(mkCombo(m_page1, swN[i]), i, 1);
    }
    p1->addLayout(gSw);

    // --- Педали ---
    p1->addLayout(sectionHeader(m_page1, tr("TR_Ftstch")));
    QGridLayout *gPed = new QGridLayout();
    gPed->addWidget(mkLbl(m_page1, tr("TR_Lt:"), "label_pedal1"), 0, 0);
    gPed->addWidget(mkCombo(m_page1, "cmb_pedal1"), 0, 1);
    gPed->addWidget(mkLbl(m_page1, tr("TR_Rt:"), "label_pedal2"), 1, 0);
    gPed->addWidget(mkCombo(m_page1, "cmb_pedal2"), 1, 1);
    p1->addLayout(gPed);

    root->addWidget(m_page1);

    // ===================== СТРАНИЦА 2 (скрыта) =====================
    m_page2 = new QFrame(host);
    m_page2->setObjectName(QStringLiteral("frame_page2"));
    m_page2->setFrameShape(QFrame::StyledPanel);
    m_page2->setFrameShadow(QFrame::Raised);
    QVBoxLayout *p2 = new QVBoxLayout(m_page2);
    p2->addLayout(sectionHeader(m_page2, tr("TR_Otr")));
    QGridLayout *gO = new QGridLayout();
    gO->addWidget(mkLbl(m_page2, tr("TR_IRIS1:"), "label_IRIS"), 0, 0);
    gO->addWidget(mkCombo(m_page2, "cmb_iris"), 0, 1, 1, 2);
    gO->addWidget(mkLbl(m_page2, tr("TR_IDenoise"), "label"), 1, 0);
    gO->addWidget(mkCombo(m_page2, "cmb_denoise"), 1, 1, 1, 2);
    QLabel *lbEq = mkLbl(m_page2, tr("TR_BEquilibria"), "label_4"); lbEq->setWordWrap(true);
    gO->addWidget(lbEq, 2, 0);
    gO->addWidget(mkCombo(m_page2, "cmb_brightEQ"), 2, 1, 1, 2);
    QLabel *lbSr = mkLbl(m_page2, tr("TR_SRemoval"), "label_3"); lbSr->setWordWrap(true);
    gO->addWidget(lbSr, 3, 0);
    gO->addWidget(mkCombo(m_page2, "optionbtn_dehaze"), 3, 1, 1, 2);   // реф. KOptionListButton
    gO->addWidget(mkLbl(m_page2, QStringLiteral("HDR"), "label_5"), 4, 0);
    gO->addWidget(mkCombo(m_page2, "optionbtn_hdr"), 4, 1, 1, 2);      // реф. KOptionListButton
    p2->addLayout(gO);
    m_page2->hide();                       // реф. frame_page2 скрыт по умолчанию
    root->addWidget(m_page2);

    root->addStretch(1);

    // Реф. btn_nextpage ">" — переключение страниц (SwitchShowPage). Чистый UI.
    QPushButton *btnNext = new QPushButton(QStringLiteral(">"), host);
    btnNext->setObjectName(QStringLiteral("btn_nextpage"));
    btnNext->setFixedWidth(60);
    connect(btnNext, &QPushButton::clicked, this, [this]() {
        bool p1vis = m_page1->isVisible();
        m_page1->setVisible(!p1vis);
        m_page2->setVisible(p1vis);
    });
    QHBoxLayout *hNext = new QHBoxLayout();
    hNext->addStretch(1);
    hNext->addWidget(btnNext);
    root->addLayout(hNext);

    // Реф. frame_btn (KParamSetBtn) — панель Default/Save/Exit.
    QHBoxLayout *hb = new QHBoxLayout();
    hb->setObjectName(QStringLiteral("frame_btn"));
    auto mkBtn = [&](const char *name, const QString &text) {
        QPushButton *b = new QPushButton(host);
        b->setObjectName(QString::fromLatin1(name));
        b->setText(text);
        b->setMinimumWidth(100);
        hb->addWidget(b);
        return b;
    };
    hb->addStretch(1);
    mkBtn("btn_default", tr("TR_Dflt"));   // реф. ClickBtnDefault (device commit)
    mkBtn("btn_save", tr("TR_Sve"));       // реф. ClickBtnSave (device commit)
    QPushButton *btnExit = mkBtn("btn_exit", tr("TR_Ext"));
    connect(btnExit, &QPushButton::clicked, this, &QWidget::close);   // реф. ClickBtnClose
    hb->addStretch(1);
    root->addLayout(hb);

    // Реф. line_foot (KLineH) — нижний разделитель.
    root->addWidget(mkLineH(host));

    // Все 17 слотов Change* (ConnectEditSignal) — DEVICE, не подключаем.
}
