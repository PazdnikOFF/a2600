#include "KPrintSettingsDlg.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>

namespace {
QLabel *capLbl(QWidget *p, const QString &text, const char *name)
{
    QLabel *l = new QLabel(text, p);
    l->setObjectName(QString::fromLatin1(name));
    l->setMaximumWidth(150);
    l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);   // реф. align 0x82
    return l;
}
QPushButton *stepBtn(QWidget *p, const char *name, const QString &glyph)
{
    QPushButton *b = new QPushButton(glyph, p);
    b->setObjectName(QString::fromLatin1(name));
    b->setFixedSize(25, 25);   // реф. fixed 25×25
    return b;
}
QSlider *mkSlider(QWidget *p, const char *name, int lo, int hi)   // реф. KMySlider → QSlider
{
    QSlider *s = new QSlider(Qt::Horizontal, p);
    s->setObjectName(QString::fromLatin1(name));
    s->setTickPosition(QSlider::TicksBothSides);   // реф. tickPos=3
    s->setRange(lo, hi);
    s->setMinimumWidth(250);   // реф. 250 (гориз. протяжённость)
    return s;
}
} // namespace

KPrintSettingsDlg::KPrintSettingsDlg(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x78ec10: KDialog(modal=false) → setupUi → SetTitle(TR_PESettings) →
    // setStyleSheet(тёмный) → диапазоны слайдеров → KPrinterManager (device) →
    // RegisterSignalConnect → RenderPixmapOfImageLabel (device).
    setupUi();
    SetTitle(tr("TR_PESettings"));
}

void KPrintSettingsDlg::setupUi()
{
    setObjectName(QStringLiteral("KPrintSettingsDlg"));

    QWidget *host = ContentArea();
    host->setStyleSheet(QStringLiteral(
        "QWidget{background-color: rgba(26,26,26,255);}"));   // реф. тёмный фон
    QVBoxLayout *root = new QVBoxLayout(host);

    // Превью снимка (device — RenderPixmapOfImageLabel) → плейсхолдер.
    QLabel *imgLbl = new QLabel(host);
    imgLbl->setObjectName(QStringLiteral("m_pImageLbl"));
    imgLbl->setMinimumSize(768, 400);   // реф. ≈768×400
    imgLbl->setAlignment(Qt::AlignCenter);
    imgLbl->setStyleSheet(QStringLiteral("border:1px solid rgb(83,83,83);color:rgb(120,120,120);"));
    imgLbl->setText(tr("TR_PESettings"));
    root->addWidget(imgLbl, 0, Qt::AlignHCenter);

    // === Сетка принтера ===
    QGridLayout *gPr = new QGridLayout();
    gPr->setObjectName(QStringLiteral("gridLayout"));
    gPr->addWidget(capLbl(host, tr("TR_PName"), "m_pPrinterName"), 0, 0);
    QComboBox *cmbPrinter = new QComboBox(host);   // реф. device-список (KPrinterManager)
    cmbPrinter->setObjectName(QStringLiteral("m_pPrinterNameCmb"));
    cmbPrinter->setMaximumWidth(600);
    cmbPrinter->addItem(QString());   // реф. пустой placeholder
    gPr->addWidget(cmbPrinter, 0, 2);
    gPr->addWidget(capLbl(host, tr("TR_RPSize"), "m_pPageSizeLbl"), 1, 0);
    QComboBox *cmbPaper = new QComboBox(host);
    cmbPaper->setObjectName(QStringLiteral("m_pPaperSizeCmb"));
    cmbPaper->setMaximumWidth(600);
    cmbPaper->addItem(QStringLiteral("A4(210mm*297mm)"));       // реф. статичные пункты
    cmbPaper->addItem(QStringLiteral("Carta(216mm*279mm)"));
    gPr->addWidget(cmbPaper, 1, 2);
    gPr->addWidget(capLbl(host, tr("TR_IPEffect"), "m_pOptLbl"), 2, 0);
    QCheckBox *chkOpt = new QCheckBox(tr("TR_IOptimization"), host);
    chkOpt->setObjectName(QStringLiteral("m_pOptChkBox"));
    chkOpt->setChecked(false);   // реф. initial unchecked
    gPr->addWidget(chkOpt, 2, 2);
    gPr->setColumnStretch(1, 1);
    root->addLayout(gPr);

    // === Сетка слайдеров ===
    QGridLayout *gSl = new QGridLayout();
    gSl->setObjectName(QStringLiteral("gridLayout_2"));
    gSl->addWidget(capLbl(host, tr("TR_Brtnss"), "m_pBrightLbl"), 0, 0);
    gSl->addWidget(stepBtn(host, "m_pBrightDecreaseBtn", QStringLiteral("-")), 0, 2);
    gSl->addWidget(mkSlider(host, "m_pBrightSlider", -100, 100), 0, 3);   // реф. [-100,100]
    gSl->addWidget(stepBtn(host, "m_pBrightIncreaseBtn", QStringLiteral("+")), 0, 4);
    gSl->addWidget(capLbl(host, tr("TR_Gma"), "m_pGammaLbl"), 1, 0);
    gSl->addWidget(stepBtn(host, "m_pGammaDecreaseBtn", QStringLiteral("-")), 1, 2);
    gSl->addWidget(mkSlider(host, "m_pGammaSlider", 0, 2000), 1, 3);      // реф. [0,2000]
    gSl->addWidget(stepBtn(host, "m_pGammaIncreaseBtn", QStringLiteral("+")), 1, 4);
    gSl->setColumnStretch(1, 1);
    root->addLayout(gSl);

    root->addStretch(1);

    // === Ряд кнопок ===
    QHBoxLayout *hBtn = new QHBoxLayout();
    hBtn->setObjectName(QStringLiteral("horizontalLayout"));
    QPushButton *btnOk = new QPushButton(tr("TR_OK"), host);
    btnOk->setObjectName(QStringLiteral("m_pOkBtn"));
    QPushButton *btnCancel = new QPushButton(tr("TR_Ccl"), host);
    btnCancel->setObjectName(QStringLiteral("m_pCancelBtn"));
    QPushButton *btnDefault = new QPushButton(tr("TR_Dflt"), host);
    btnDefault->setObjectName(QStringLiteral("m_pLoadDefaultBtn"));   // реф. LoadDefault — device
    hBtn->addWidget(btnOk);
    hBtn->addStretch(1);
    hBtn->addWidget(btnCancel);
    hBtn->addStretch(1);
    hBtn->addWidget(btnDefault);
    root->addLayout(hBtn);

    connect(btnOk, &QPushButton::clicked, this, &QWidget::close);       // реф. OnOkBtnClicked (accept)
    connect(btnCancel, &QPushButton::clicked, this, &QWidget::close);   // реф. OnCancelBtnClicked (reject)
    // Слайдеры/±/LoadDefault/printer-change/превью — DEVICE, не подключаем.
}
