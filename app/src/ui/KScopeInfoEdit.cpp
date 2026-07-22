#include "KScopeInfoEdit.h"
#include "KLineH.h"

#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

namespace {
KLineH *mkLineH(QWidget *p)   // АПГРЕЙД: реальный KLineH (line_espec/einfo/uinfo) — был QFrame
{
    return new KLineH(p);
}
QHBoxLayout *sectionHeader(QWidget *p, const QString &text)
{
    QHBoxLayout *h = new QHBoxLayout();
    QLabel *l = new QLabel(text, p);
    l->setStyleSheet(QStringLiteral("font-weight:bold;"));
    h->addWidget(l);
    h->addWidget(mkLineH(p), 1);
    return h;
}
} // namespace

KScopeInfoEdit::KScopeInfoEdit(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x644cf0: KDialog(modal=false) → setupUi → SetKStyle(2) → title TR_EInfo →
    // InitWidgetStatus → device-connects → LoadScopeInfo (device) → AllConnect → saveTimer.
    setupUi();
    SetKStyle(KDLG_W460);              // реф. SetKStyle(2)
    SetTitle(tr("TR_EInfo"));
}

void KScopeInfoEdit::setupUi()
{
    setObjectName(QStringLiteral("KScopeInfoEdit"));
    resize(600, 919);

    QWidget *host = ContentArea();
    QVBoxLayout *root = new QVBoxLayout(host);

    // ===================== frame_2: сервис-логин =====================
    QFrame *frame2 = new QFrame(host);
    frame2->setObjectName(QStringLiteral("frame_2"));
    frame2->setFrameShape(QFrame::StyledPanel); frame2->setFrameShadow(QFrame::Raised);
    QGridLayout *g4 = new QGridLayout(frame2);
    g4->setContentsMargins(9, 28, 9, 9);
    g4->addWidget(new QLabel(tr("TR_PSN:"), frame2), 0, 0);
    QLabel *lblSN = new QLabel(frame2); lblSN->setObjectName(QStringLiteral("label_SN"));
    g4->addWidget(lblSN, 0, 2, 1, 2);   // device
    g4->addWidget(new QLabel(tr("TR_PCN:"), frame2), 1, 0);
    QLabel *lblCN = new QLabel(frame2); lblCN->setObjectName(QStringLiteral("label_CN"));
    g4->addWidget(lblCN, 1, 2, 1, 2);   // device
    QPushButton *btnLogin = new QPushButton(tr("TR_DLogin"), frame2);
    btnLogin->setObjectName(QStringLiteral("btn_userlogin"));
    g4->addWidget(btnLogin, 2, 0, 1, 2);
    QPushButton *btnLogout = new QPushButton(tr("TR_Lgt"), frame2);
    btnLogout->setObjectName(QStringLiteral("btn_logout"));
    g4->addWidget(btnLogout, 2, 2);
    g4->addWidget(new QLabel(tr("TR_ESN:"), frame2), 3, 0);
    QLineEdit *importSn = new QLineEdit(frame2); importSn->setObjectName(QStringLiteral("import_endosn"));
    g4->addWidget(importSn, 3, 1, 1, 2);
    QPushButton *btnImport = new QPushButton(tr("TR_Impt"), frame2);
    btnImport->setObjectName(QStringLiteral("importAuthBin"));   // device
    g4->addWidget(btnImport, 3, 3);
    root->addWidget(frame2);

    // ===================== Тип =====================
    QHBoxLayout *h6 = new QHBoxLayout();
    QLabel *lblType = new QLabel(tr("TR_Tpe:"), host); lblType->setMinimumWidth(125);
    h6->addWidget(lblType);
    QComboBox *cmbType = new QComboBox(host); cmbType->setObjectName(QStringLiteral("cmb_type"));   // device
    h6->addWidget(cmbType);
    root->addLayout(h6);

    // ===================== Спека =====================
    root->addLayout(sectionHeader(host, tr("TR_ESpecification")));
    QGridLayout *g = new QGridLayout();
    auto roEdit = [&](const char *name) {
        QLineEdit *e = new QLineEdit(host); e->setObjectName(QString::fromLatin1(name));
        e->setDisabled(true); e->setReadOnly(true); e->setFocusPolicy(Qt::NoFocus);   // реф. disabled+RO+NoFocus
        return e;
    };
    auto diaSpin = [&](const char *name) {   // реф. Φ ... mm, dec1, max25.5, step0.1
        QDoubleSpinBox *s = new QDoubleSpinBox(host); s->setObjectName(QString::fromLatin1(name));
        s->setDecimals(1); s->setSingleStep(0.1); s->setMaximum(25.5);
        s->setPrefix(QString(QChar(0x03A6)) + QStringLiteral(" ")); s->setSuffix(QStringLiteral("mm"));
        return s;
    };
    g->addWidget(new QLabel(tr("TR_EModel:"), host), 0, 0);
    QHBoxLayout *hm = new QHBoxLayout();
    hm->addWidget(roEdit("edt_model"));
    QComboBox *cmbModel = new QComboBox(host); cmbModel->setObjectName(QStringLiteral("cmb_model"));   // device
    hm->addWidget(cmbModel);
    g->addLayout(hm, 0, 1);
    g->addWidget(new QLabel(tr("TR_EType:"), host), 1, 0);
    QHBoxLayout *ht = new QHBoxLayout();
    ht->addWidget(roEdit("edt_endoscope_type"));
    QComboBox *cmbEType = new QComboBox(host); cmbEType->setObjectName(QStringLiteral("cmb_endoscope_type"));   // device
    ht->addWidget(cmbEType);
    g->addLayout(ht, 1, 1);
    g->addWidget(new QLabel(tr("TR_MIDOTIChannel:"), host), 2, 0);
    g->addWidget(diaSpin("spin_inschannel"), 2, 1);
    g->addWidget(new QLabel(tr("TR_ODODEnd:"), host), 3, 0);
    g->addWidget(diaSpin("spin_distalend"), 3, 1);
    g->addWidget(new QLabel(tr("TR_ODOBSection:"), host), 4, 0);
    g->addWidget(diaSpin("spin_bsection"), 4, 1);
    g->addWidget(new QLabel(tr("TR_WLength:"), host), 5, 0);
    QSpinBox *spWork = new QSpinBox(host); spWork->setObjectName(QStringLiteral("spin_worklength"));
    spWork->setMaximum(5000); spWork->setSingleStep(5); spWork->setSuffix(QStringLiteral("mm"));
    g->addWidget(spWork, 5, 1);
    root->addLayout(g);

    // ===================== Статус-виджет + stacked =====================
    QHBoxLayout *h2 = new QHBoxLayout();
    QFrame *scopeStatus = new QFrame(host);   // реф. KScopeStaus → плейсхолдер
    scopeStatus->setObjectName(QStringLiteral("frame_scopestaus"));
    scopeStatus->setFrameShape(QFrame::StyledPanel); scopeStatus->setFrameShadow(QFrame::Raised);
    scopeStatus->setFixedWidth(125);
    h2->addWidget(scopeStatus);
    QStackedWidget *stk = new QStackedWidget(host); stk->setObjectName(QStringLiteral("stackedWidget"));
    stk->addWidget(new QWidget(stk)); stk->addWidget(new QWidget(stk));   // page/page_2 (камера/скоп)
    h2->addWidget(stk);
    h2->addStretch(1);
    root->addLayout(h2);

    // ===================== frame_manu: произв. инфо =====================
    root->addLayout(sectionHeader(host, tr("TR_EInfo")));
    QFrame *frameManu = new QFrame(host); frameManu->setObjectName(QStringLiteral("frame_manu"));
    QGridLayout *g2 = new QGridLayout(frameManu); g2->setContentsMargins(0, 0, 0, 0);
    g2->addWidget(new QLabel(tr("TR_MDate:"), frameManu), 0, 0);
    QDateEdit *dateManu = new QDateEdit(frameManu); dateManu->setObjectName(QStringLiteral("date_manu"));
    dateManu->setDateRange(QDate(1970, 1, 1), QDate(2050, 12, 31));   // реф. 1970..2050
    dateManu->setCalendarPopup(true); dateManu->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    g2->addWidget(dateManu, 0, 1);
    QLabel *lblEsn = new QLabel(tr("TR_ESN:"), frameManu); lblEsn->setMinimumWidth(125);
    g2->addWidget(lblEsn, 1, 0);
    QLineEdit *edtEsn = new QLineEdit(frameManu); edtEsn->setObjectName(QStringLiteral("edt_esn"));
    edtEsn->setReadOnly(true);
    g2->addWidget(edtEsn, 1, 1);
    g2->addWidget(new QLabel(tr("TR_FOUse:"), frameManu), 2, 0);
    QLineEdit *edtFreq = new QLineEdit(frameManu); edtFreq->setObjectName(QStringLiteral("edt_freq"));
    edtFreq->setReadOnly(true);
    g2->addWidget(edtFreq, 2, 1);
    root->addWidget(frameManu);

    // ===================== frame_user: польз. инфо =====================
    root->addLayout(sectionHeader(host, tr("TR_UInfo")));
    QFrame *frameUser = new QFrame(host); frameUser->setObjectName(QStringLiteral("frame_user"));
    QGridLayout *g3 = new QGridLayout(frameUser); g3->setContentsMargins(0, 0, 0, 0);
    g3->addWidget(new QLabel(tr("TR_CNo:"), frameUser), 0, 0);
    QLineEdit *edtCno = new QLineEdit(frameUser); edtCno->setObjectName(QStringLiteral("edt_contrastno"));
    edtCno->setMaxLength(16);
    g3->addWidget(edtCno, 0, 1);
    g3->addWidget(new QLabel(tr("TR_Cmnt:"), frameUser), 1, 0);
    QLineEdit *edtCmt = new QLineEdit(frameUser); edtCmt->setObjectName(QStringLiteral("edt_comment"));
    edtCmt->setMaxLength(20);
    g3->addWidget(edtCmt, 1, 1);
    root->addWidget(frameUser);

    root->addStretch(1);

    // ===================== Кнопки =====================
    QHBoxLayout *hTool = new QHBoxLayout();
    QPushButton *btnSave = new QPushButton(tr("TR_Sve"), host);
    btnSave->setObjectName(QStringLiteral("btn_save"));
    btnSave->setMinimumWidth(100); btnSave->setMaximumWidth(120);   // реф. OnSave (device)
    QPushButton *btnExit = new QPushButton(tr("TR_Ext"), host);
    btnExit->setObjectName(QStringLiteral("btn_exit"));
    btnExit->setFixedWidth(120);
    hTool->addStretch(1);
    hTool->addWidget(btnSave);
    hTool->addWidget(btnExit);
    hTool->addStretch(1);
    root->addLayout(hTool);

    connect(btnExit, &QPushButton::clicked, this, &QWidget::close);   // реф. OnExit→close
}
