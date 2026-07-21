#include "KCameraInfoEdit.h"

#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegExpValidator>
#include <QVBoxLayout>
#include <QWidget>

KCameraInfoEdit::KCameraInfoEdit(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x5fdf58: KDialog(modal=false) → setupUi → SetKStyle(2) → title TR_CHInfo →
    // InitWidgetStatus (комбо+роль-гейтинг, device) → LoadScopeInfo → AllConnect → saveTimer.
    setupUi();
    SetKStyle(KDLG_W460);              // реф. SetKStyle(2)
    SetTitle(tr("TR_CHInfo"));
}

void KCameraInfoEdit::setupUi()
{
    setObjectName(QStringLiteral("KCameraInfoEdit"));
    resize(600, 843);

    QWidget *host = ContentArea();
    QVBoxLayout *root = new QVBoxLayout(host);
    root->setContentsMargins(20, 40, 20, 9);   // реф. margins (20,40,…)

    // ===================== frame_2: сервис-логин =====================
    QFrame *frame2 = new QFrame(host);
    frame2->setObjectName(QStringLiteral("frame_2"));
    frame2->setFrameShape(QFrame::StyledPanel); frame2->setFrameShadow(QFrame::Raised);
    QGridLayout *g4 = new QGridLayout(frame2);
    g4->setContentsMargins(9, 12, 9, 9);
    g4->addWidget(new QLabel(tr("TR_PSN:"), frame2), 0, 0);
    QLabel *lblSN = new QLabel(frame2); lblSN->setObjectName(QStringLiteral("label_SN"));
    g4->addWidget(lblSN, 0, 2, 1, 2);   // device: GetProcessorSN
    g4->addWidget(new QLabel(tr("TR_PCN:"), frame2), 1, 0);
    QLabel *lblCN = new QLabel(frame2); lblCN->setObjectName(QStringLiteral("label_CN"));
    g4->addWidget(lblCN, 1, 2, 1, 2);   // device: GetProductCN
    QPushButton *btnLogin = new QPushButton(tr("TR_DLogin"), frame2);
    btnLogin->setObjectName(QStringLiteral("btn_userlogin"));
    g4->addWidget(btnLogin, 2, 0, 1, 2);
    QPushButton *btnLogout = new QPushButton(tr("TR_Lgt"), frame2);
    btnLogout->setObjectName(QStringLiteral("btn_logout"));
    g4->addWidget(btnLogout, 2, 2);
    g4->addWidget(new QLabel(tr("TR_CHSN:"), frame2), 3, 0);
    QLineEdit *importSn = new QLineEdit(frame2); importSn->setObjectName(QStringLiteral("import_endosn"));
    g4->addWidget(importSn, 3, 1, 1, 2);
    QPushButton *btnImport = new QPushButton(tr("TR_Impt"), frame2);
    btnImport->setObjectName(QStringLiteral("importAuthBin"));   // device
    g4->addWidget(btnImport, 3, 3);
    root->addWidget(frame2);

    // ===================== Тип (статичный SLens/HLens) =====================
    QHBoxLayout *h3 = new QHBoxLayout();
    QLabel *lblType = new QLabel(tr("TR_Tpe:"), host); lblType->setMinimumWidth(125);
    h3->addWidget(lblType);
    QComboBox *cmbType = new QComboBox(host); cmbType->setObjectName(QStringLiteral("cmb_type"));
    cmbType->addItem(tr("TR_SLens"), 0);   // реф. InitWidgetStatus: статично 0/1
    cmbType->addItem(tr("TR_HLens"), 1);
    h3->addWidget(cmbType);
    root->addLayout(h3);

    // ===================== Сетка model/esn =====================
    QGridLayout *g = new QGridLayout();
    g->addWidget(new QLabel(tr("TR_CHModel:"), host), 0, 0);
    QHBoxLayout *hm = new QHBoxLayout();
    QLineEdit *edtModel = new QLineEdit(host); edtModel->setObjectName(QStringLiteral("edt_model"));
    edtModel->setReadOnly(true); edtModel->setDisabled(true); edtModel->setFocusPolicy(Qt::NoFocus);
    hm->addWidget(edtModel);
    QComboBox *cmbModel = new QComboBox(host); cmbModel->setObjectName(QStringLiteral("cmb_model"));   // device
    hm->addWidget(cmbModel);
    g->addLayout(hm, 0, 1);
    QLabel *lblEsn = new QLabel(tr("TR_CHSN:"), host); lblEsn->setMinimumWidth(125);
    g->addWidget(lblEsn, 1, 0);
    QHBoxLayout *he = new QHBoxLayout();
    QLineEdit *edtEsn = new QLineEdit(host); edtEsn->setObjectName(QStringLiteral("edt_esn"));
    edtEsn->setMaxLength(10);   // реф. maxLen 10
    edtEsn->setValidator(new QRegExpValidator(
        QRegExp(QStringLiteral("[A-Za-z0-9]{0,10}")), edtEsn));   // реф. валидатор
    he->addWidget(edtEsn);
    g->addLayout(he, 1, 1);
    root->addLayout(g);

    // ===================== frame_manu: пустой плейсхолдер =====================
    QFrame *frameManu = new QFrame(host);
    frameManu->setObjectName(QStringLiteral("frame_manu"));
    new QGridLayout(frameManu);   // реф. gridLayout_2 пустой
    root->addWidget(frameManu);

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
