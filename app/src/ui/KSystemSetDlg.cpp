#include "KSystemSetDlg.h"
#include "KIpLineEdit.h"
#include "KLineH.h"

#include <QComboBox>
#include <QDateEdit>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpacerItem>
#include <QTimeEdit>
#include <QVBoxLayout>
#include <QWidget>

namespace {
// Строка-заголовок секции: подпись + горизонтальный разделитель (реф. hLayout_* + KLineH).
// АПГРЕЙД: реальный KLineH (line_zoom/line_color/line_enhance) — был QFrame(HLine/Sunken).
QHBoxLayout *sectionHeader(QWidget *p, const QString &text)
{
    QHBoxLayout *h = new QHBoxLayout();
    QLabel *l = new QLabel(p);
    l->setText(text);
    l->setStyleSheet(QStringLiteral("font-weight:bold;"));
    h->addWidget(l);
    h->addWidget(new KLineH(p), 1);
    return h;
}
// АПГРЕЙД: реальный KIpLineEdit (edt_localip/netmask/gateway) — был QLineEdit c маской.
KIpLineEdit *mkIpEdit(QWidget *p, const char *name)
{
    KIpLineEdit *e = new KIpLineEdit(p);
    e->setObjectName(QString::fromLatin1(name));
    return e;
}
} // namespace

KSystemSetDlg::KSystemSetDlg(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x5f5ed0: setupUi → SetKStyle(2) → title TR_SSettings → initWidget →
    // initVar (device) → connects → QTimer(1с)→UpdateDateTime → eventFilter на комбо.
    setupUi();
    SetKStyle(KDLG_W460);              // реф. SetKStyle(2)
    SetTitle(tr("TR_SSettings"));      // реф. перекрывает setupUi-титул "TR_Dlg"
}

void KSystemSetDlg::setupUi()
{
    setObjectName(QStringLiteral("KSystemSetDlg"));
    resize(566, 1080);

    QWidget *host = ContentArea();
    QVBoxLayout *root = new QVBoxLayout(host);
    root->setObjectName(QStringLiteral("verticalLayout"));
    root->setContentsMargins(9, 40, 9, 9);   // реф. верхний отступ ~40

    // Реф. frame_page1 (StyledPanel/Raised) → gridLayout_2.
    QFrame *frame_page1 = new QFrame(host);
    frame_page1->setObjectName(QStringLiteral("frame_page1"));
    frame_page1->setFrameShape(QFrame::StyledPanel);
    frame_page1->setFrameShadow(QFrame::Raised);
    QGridLayout *g = new QGridLayout(frame_page1);
    g->setObjectName(QStringLiteral("gridLayout_2"));
    g->setColumnMinimumWidth(0, 20);   // реф. col0 = левый отступ-спейсер
    g->setColumnStretch(3, 1);         // контролы в col3 тянутся

    int r = 0;
    auto addRow = [&](const QString &cap, QWidget *ctl, const char *labelName) {
        QLabel *l = new QLabel(cap, frame_page1);
        l->setObjectName(QString::fromLatin1(labelName));
        g->addWidget(l, r, 1);
        g->addWidget(ctl, r, 3);
        ++r;
    };
    auto addHeader = [&](const QString &text) {
        g->addLayout(sectionHeader(frame_page1, text), r, 0, 1, 4);
        ++r;
    };

    // === Секция «Видео/UI» (реф. label_enhance = TR_Vdeo2) ===
    addHeader(tr("TR_Vdeo2"));
    QComboBox *cmbRes = new QComboBox(frame_page1);
    cmbRes->setObjectName(QStringLiteral("cmb_resolution"));
    cmbRes->addItem(QStringLiteral("1920x1080"));   // реф. литералы (не TR)
    cmbRes->addItem(QStringLiteral("1920x1200"));
    addRow(tr("TR_Rslton:"), cmbRes, "label_resolution");

    QComboBox *cmbCorner = new QComboBox(frame_page1);
    cmbCorner->setObjectName(QStringLiteral("cmb_connershape"));
    cmbCorner->addItem(tr("TR_Rnd"));
    cmbCorner->addItem(tr("TR_Ocgle"));
    addRow(tr("TR_CShape:"), cmbCorner, "label_connershape");

    QComboBox *cmbCut = new QComboBox(frame_page1);
    cmbCut->setObjectName(QStringLiteral("cmb_cornerCutSize"));
    cmbCut->addItem(tr("TR_Big"), 0);
    cmbCut->addItem(tr("TR_Sml"), 1);
    addRow(tr("TR_BMode"), cmbCut, "label_cornerCutSize");

    QComboBox *cmbVls = new QComboBox(frame_page1);   // реф. InitVlsConfigSet — device
    cmbVls->setObjectName(QStringLiteral("cmb_vlsconfig"));
    addRow(tr("TR_LMode:"), cmbVls, "label_vlsconfig");

    QComboBox *cmbSeg = new QComboBox(frame_page1);
    cmbSeg->setObjectName(QStringLiteral("cmb_videoSegmentation"));
    cmbSeg->addItem(tr("TR_Inf"));   // реф. + GetSaveVideoSplitList (device) — опущено
    addRow(tr("TR_VSegmentation:"), cmbSeg, "label_videoSegmentation");

    // === Секция «Язык/Дата» (реф. label_color = TR_LATime) ===
    addHeader(tr("TR_LATime"));
    QComboBox *cmbLang = new QComboBox(frame_page1);
    cmbLang->setObjectName(QStringLiteral("cmb_language"));
    const char *langs[] = {"TR_Chse", "TR_Esh", "TR_Spsh", "TR_Itln",
                           "TR_Frch", "TR_Rssn", "TR_Gmn", "TR_Plsh"};
    for (const char *t : langs)
        cmbLang->addItem(tr(t));
    addRow(tr("TR_Lge:"), cmbLang, "label_vlsconfig_2");

    QComboBox *cmbDateFmt = new QComboBox(frame_page1);   // реф. GetDateFormatList — device
    cmbDateFmt->setObjectName(QStringLiteral("cmb_dateformat"));
    addRow(tr("TR_Tpe1:"), cmbDateFmt, "labe_dateformat");

    QDateEdit *dateEdit = new QDateEdit(frame_page1);
    dateEdit->setObjectName(QStringLiteral("dateEdit"));
    dateEdit->setCalendarPopup(true);
    addRow(tr("TR_Dte:"), dateEdit, "label_date");

    QTimeEdit *timeEdit = new QTimeEdit(frame_page1);
    timeEdit->setObjectName(QStringLiteral("timeEdit"));
    addRow(tr("TR_Tme:"), timeEdit, "label_time");

    // === Секция «Сеть» (реф. label_zoom = TR_Ntwrk) ===
    addHeader(tr("TR_Ntwrk"));
    addRow(tr("TR_LIP:"), mkIpEdit(frame_page1, "edt_localip"), "label_ip");
    addRow(tr("TR_Ntsk:"), mkIpEdit(frame_page1, "edt_localnetmask"), "label_netmask");
    addRow(tr("TR_Gtwy:"), mkIpEdit(frame_page1, "edt_localgateway"), "label_gateway");
    QLineEdit *edtMac = new QLineEdit(frame_page1);
    edtMac->setObjectName(QStringLiteral("edt_macAddress"));
    edtMac->setEnabled(false);   // реф. включён только для роли ≥4 (device) — заглушка
    addRow(tr("TR_Maddress:"), edtMac, "label_macAddressTips");

    root->addWidget(frame_page1);
    root->addStretch(1);   // реф. вертикальный спейсер 0x148

    // Реф. label_message — «требуется перезапуск», центр, wordWrap.
    QLabel *label_message = new QLabel(host);
    label_message->setObjectName(QStringLiteral("label_message"));
    label_message->setText(tr("TR_TITEARestart"));
    label_message->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    label_message->setWordWrap(true);
    root->addWidget(label_message);

    // === Панель кнопок (реф. frame_btn = KParamSetBtn) ===
    // Реф. SetBtnCustomFuncName: Общие(F2)/Параметры(F3)/Устройство(F4) — переключение
    // диалогов (SwitchDialog), + Save/Default/Exit. У нас — обычная панель.
    QFrame *frame_btn = new QFrame(host);
    frame_btn->setObjectName(QStringLiteral("frame_btn"));
    QHBoxLayout *hb = new QHBoxLayout(frame_btn);
    auto mkBtn = [&](const char *name, const QString &text) {
        QPushButton *b = new QPushButton(frame_btn);
        b->setObjectName(QString::fromLatin1(name));
        b->setText(text);
        b->setMinimumWidth(100);
        hb->addWidget(b);
        return b;
    };
    mkBtn("btn_general", tr("TR_Gnrl4") + QStringLiteral(" (F2)"));
    mkBtn("btn_param", tr("TR_PSET2") + QStringLiteral(" (F3)"));
    mkBtn("btn_device", tr("TR_DSetting") + QStringLiteral(" (F4)"));
    hb->addStretch(1);
    mkBtn("btn_default", tr("TR_Dflt"));   // реф. ClickBtnDefault → LoadDefaultConfig (device)
    mkBtn("btn_save", tr("TR_Sve"));       // реф. ClickBtnSave → SaveConfig (device)
    QPushButton *btnExit = mkBtn("btn_exit", tr("TR_Ext"));
    connect(btnExit, &QPushButton::clicked, this, &QWidget::close);   // реф. ClickBtnClose
    root->addWidget(frame_btn);
}
