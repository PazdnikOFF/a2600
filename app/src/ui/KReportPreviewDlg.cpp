#include "KReportPreviewDlg.h"

#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRegExpValidator>
#include <QVBoxLayout>
#include <QWidget>

KReportPreviewDlg::KReportPreviewDlg(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x500d08: KFullScreenDialog(parent,-1) → Create → setupUi →
    // SetTitle(TR_Prvw2) → ConfigCentralView (рендер-вьюха, device) → InitConnect.
    setupUi();
    SetKStyle(KDLG_FULLSCREEN);        // реф. полноэкранная страница
    SetTitle(tr("TR_Prvw2"));
}

void KReportPreviewDlg::setupUi()
{
    setObjectName(QStringLiteral("KReportPreviewDlg"));

    QWidget *host = ContentArea();
    QHBoxLayout *root = new QHBoxLayout(host);
    root->setObjectName(QStringLiteral("horizontalLayout"));
    root->setContentsMargins(0, 46, 0, 0);

    // ===================== Левый тулбар =====================
    QWidget *leftW = new QWidget(host);
    leftW->setObjectName(QStringLiteral("verticalWidget_left"));
    leftW->setStyleSheet(QStringLiteral(
        "QWidget#verticalWidget_left{background-color:rgb(29,31,35);}"));
    QVBoxLayout *vL = new QVBoxLayout(leftW);
    vL->setObjectName(QStringLiteral("verticalLayout_left"));
    vL->setContentsMargins(15, 22, 15, 22);
    auto addBtn = [&](const char *name, const QString &text) {
        QPushButton *b = new QPushButton(text, leftW);
        b->setObjectName(QString::fromLatin1(name));
        b->setFixedWidth(211);   // реф. min=max=211
        vL->addWidget(b);
        return b;
    };
    addBtn("btn_fit_width", tr("TR_FWidth") + QStringLiteral(" (F1)"));   // реф. → рендер (fit)
    addBtn("btn_fit_page", tr("TR_FPage") + QStringLiteral(" (F2)"));
    addBtn("btn_print", tr("TR_Prnt") + QStringLiteral(" (F3)"));         // реф. OnBtnPrint (device)
    QFrame *line = new QFrame(leftW);
    line->setObjectName(QStringLiteral("line"));
    line->setFrameShape(QFrame::HLine);   // реф. frameShape=4 (HLine) — сепаратор в vbox
    line->setFrameShadow(QFrame::Sunken);
    vL->addWidget(line);
    QPushButton *btnZoomIn = addBtn("btn_zoom_in", tr("TR_Zmn") + QStringLiteral(" (F4)"));
    QPushButton *btnZoomOut = addBtn("btn_zoom_out", tr("TR_Zmot") + QStringLiteral(" (F5)"));
    vL->addSpacing(20);   // реф. фикс. спейсер

    // Масштаб — 29 статичных % + editable + regex-валидатор.
    QComboBox *cmbScale = new QComboBox(leftW);
    cmbScale->setObjectName(QStringLiteral("comboBox_scale"));
    cmbScale->setFixedWidth(211);
    cmbScale->setEditable(true);
    cmbScale->setInsertPolicy(QComboBox::NoInsert);
    const char *scales[] = {
        "12.5%", "25%", "30%", "40%", "50%", "60%", "70%", "80%", "90%", "100%",
        "110%", "120%", "130%", "140%", "150%", "160%", "170%", "180%", "190%", "200%",
        "250%", "300%", "400%", "500%", "600%", "700%", "800%", "900%", "999%"};
    for (const char *s : scales)
        cmbScale->addItem(QString::fromLatin1(s));
    cmbScale->setValidator(new QRegExpValidator(
        QRegExp(QStringLiteral("[0-9]{0,3}%")), cmbScale));
    cmbScale->setCurrentIndex(0);   // реф. setCurrentIndex(0)
    vL->addWidget(cmbScale);

    // Шаблон/отделение — device-список (пусто).
    QComboBox *cmbTemplate = new QComboBox(leftW);
    cmbTemplate->setObjectName(QStringLiteral("comboBox_template"));
    cmbTemplate->setFixedWidth(211);
    vL->addWidget(cmbTemplate);

    vL->addStretch(1);   // реф. вертикальный expanding — прижимает Exit вниз
    QPushButton *btnExit = addBtn("btn_exit", tr("TR_Ext"));
    connect(btnExit, &QPushButton::clicked, this, &QWidget::close);   // реф. OnExit→close

    // Zoom+/− двигают comboBox_scale (чистый UI; эффект на рендер — device).
    connect(btnZoomIn, &QPushButton::clicked, cmbScale, [cmbScale]() {
        cmbScale->setCurrentIndex(qMin(cmbScale->currentIndex() + 1, cmbScale->count() - 1));
    });
    connect(btnZoomOut, &QPushButton::clicked, cmbScale, [cmbScale]() {
        cmbScale->setCurrentIndex(qMax(cmbScale->currentIndex() - 1, 0));
    });

    root->addWidget(leftW, 235);   // реф. stretch 235

    // ===================== Область предпросмотра =====================
    QWidget *previewW = new QWidget(host);
    previewW->setObjectName(QStringLiteral("widget_preview"));
    previewW->setMinimumWidth(1281);   // реф. minW 0x501
    QGridLayout *gP = new QGridLayout(previewW);
    gP->setObjectName(QStringLiteral("gridLayout_preview"));
    gP->setContentsMargins(15, 15, 15, 15);
    // Реф. KReportPreviewCenterDlg (рендер страницы, device) → плейсхолдер.
    QLabel *center = new QLabel(previewW);
    center->setObjectName(QStringLiteral("preview_center_placeholder"));
    center->setAlignment(Qt::AlignCenter);
    center->setText(tr("TR_Prvw2"));
    center->setStyleSheet(QStringLiteral("background:rgb(20,21,25);color:rgb(120,120,120);"));
    gP->addWidget(center, 0, 0);
    root->addWidget(previewW, 1685);   // реф. stretch 1685
}
