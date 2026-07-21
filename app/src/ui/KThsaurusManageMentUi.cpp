#include "KThsaurusManageMentUi.h"

#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWidget>

KThsaurusManageMentUi::KThsaurusManageMentUi(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x4e75e8: KDialog(modal=false) → setupUi → new KThesaurusOpt (device) →
    // SetTitle(TR_Glry) → InitWidget → LoadGlryContent (device) → InitConnect.
    setupUi();
    SetTitle(tr("TR_Glry"));   // реф. перекрывает placeholder-титул "Dialog"
}

void KThsaurusManageMentUi::setupUi()
{
    setObjectName(QStringLiteral("KThsaurusManageMentUi"));

    QWidget *host = ContentArea();
    host->setMinimumSize(1024, 768);   // реф. resize(1024,768)

    QVBoxLayout *root = new QVBoxLayout(host);
    root->setObjectName(QStringLiteral("verticalLayout"));
    root->setContentsMargins(9, 41, 9, 9);   // реф. верхний отступ 41

    auto star = [&](QWidget *p) {
        QLabel *s = new QLabel(QStringLiteral("*"), p);
        s->setStyleSheet(QStringLiteral("color:#e05050;"));
        return s;
    };

    // ===================== Верхняя панель редактирования =====================
    QWidget *wEdit = new QWidget(host);
    wEdit->setObjectName(QStringLiteral("widget_glryedit"));
    QVBoxLayout *v4 = new QVBoxLayout(wEdit);

    QWidget *wGrp = new QWidget(wEdit);
    wGrp->setObjectName(QStringLiteral("widget_diseagroup"));
    QGridLayout *g = new QGridLayout(wGrp);
    g->setContentsMargins(30, 19, 45, 19);
    g->addWidget(star(wGrp), 0, 0);
    QLabel *lGrp = new QLabel(tr("TR_Grp:"), wGrp); lGrp->setObjectName(QStringLiteral("label_group"));
    g->addWidget(lGrp, 0, 1);
    QComboBox *cmbGroup = new QComboBox(wGrp);   // реф. editable, device-список
    cmbGroup->setObjectName(QStringLiteral("cmb_group"));
    cmbGroup->setEditable(true);
    g->addWidget(cmbGroup, 0, 2);
    g->addWidget(star(wGrp), 1, 0);
    QLabel *lTtl = new QLabel(tr("TR_Ttle:"), wGrp); lTtl->setObjectName(QStringLiteral("label_title"));
    g->addWidget(lTtl, 1, 1);
    QLineEdit *edTitle = new QLineEdit(wGrp);
    edTitle->setObjectName(QStringLiteral("edit_title"));
    edTitle->setMaxLength(100);   // реф. maxLen 100
    g->addWidget(edTitle, 1, 2);
    v4->addWidget(wGrp);

    QWidget *wDiag = new QWidget(wEdit);
    wDiag->setObjectName(QStringLiteral("widget_diag"));
    QVBoxLayout *v3 = new QVBoxLayout(wDiag);
    v3->setContentsMargins(30, 9, 38, 9);
    QLabel *lFind = new QLabel(tr("TR_VOExam:"), wDiag); lFind->setObjectName(QStringLiteral("label_examfinding"));
    v3->addWidget(lFind);
    QTextEdit *edFind = new QTextEdit(wDiag); edFind->setObjectName(QStringLiteral("edit_examfinding"));
    v3->addWidget(edFind);
    QLabel *lDiag = new QLabel(tr("TR_EConclusion:"), wDiag); lDiag->setObjectName(QStringLiteral("label_diagresult"));
    v3->addWidget(lDiag);
    QTextEdit *edDiag = new QTextEdit(wDiag); edDiag->setObjectName(QStringLiteral("edit_diagresult"));
    v3->addWidget(edDiag);
    v4->addWidget(wDiag);

    root->addWidget(wEdit);

    // ===================== Нижняя панель обзора =====================
    QWidget *wGlry = new QWidget(host);
    wGlry->setObjectName(QStringLiteral("widget_glry"));
    QHBoxLayout *h3 = new QHBoxLayout(wGlry);
    h3->setContentsMargins(28, 9, 28, 9);

    QWidget *wTree = new QWidget(wGlry);
    wTree->setObjectName(QStringLiteral("widget_tree"));
    QVBoxLayout *v2 = new QVBoxLayout(wTree);
    v2->setContentsMargins(15, 19, 15, 19);
    QComboBox *cmbDevice = new QComboBox(wTree);   // реф. 5 статичных типов эндоскопа
    cmbDevice->setObjectName(QStringLiteral("cmb_device"));
    const char *scopes[] = {"TR_Gspy", "TR_Cspy", "TR_Bspy", "TR_Nlspy", "TR_Cholngscpy"};
    for (const char *s : scopes)
        cmbDevice->addItem(tr(s));
    v2->addWidget(cmbDevice);
    QTreeWidget *tree = new QTreeWidget(wTree);   // реф. 1 колонка, header "1" (placeholder)
    tree->setObjectName(QStringLiteral("tree_model"));
    tree->setColumnCount(1);
    tree->setHeaderLabels({QStringLiteral("1")});
    tree->setMinimumWidth(317);
    v2->addWidget(tree);
    h3->addWidget(wTree);

    QTextEdit *edContent = new QTextEdit(wGlry);   // реф. KTextEdit → QTextEdit, disabled
    edContent->setObjectName(QStringLiteral("edit_content"));
    edContent->setEnabled(false);
    h3->addWidget(edContent, 1);

    root->addWidget(wGlry, 1);

    root->addStretch(1);
    QLabel *tip = new QLabel(tr("TR_RField"), host);
    tip->setObjectName(QStringLiteral("label_input_tip"));
    tip->setAlignment(Qt::AlignCenter);
    root->addWidget(tip);
    root->addStretch(1);

    // ===================== Ряд кнопок =====================
    QHBoxLayout *hBtn = new QHBoxLayout();
    hBtn->setObjectName(QStringLiteral("horizontalLayout"));
    auto mkBtn = [&](const char *name, const QString &text) {
        QPushButton *b = new QPushButton(text, host);
        b->setObjectName(QString::fromLatin1(name));
        b->setFixedWidth(133);
        hBtn->addWidget(b);
        return b;
    };
    mkBtn("btn_add", tr("TR_Add(F2)"));                       // реф. add — device
    QPushButton *btnMod = mkBtn("btn_modify", tr("TR_Edt(F3)"));
    QPushButton *btnCln = mkBtn("btn_clone", tr("TR_Cle(F4)"));
    QPushButton *btnDel = mkBtn("btn_delete", tr("TR_Del(D)"));
    QPushButton *btnOk = mkBtn("btn_ok", tr("TR_OK(E)"));
    QPushButton *btnCancel = mkBtn("btn_cancel", tr("TR_Ccl(E)"));
    hBtn->addStretch(1);
    root->addLayout(hBtn);

    connect(btnOk, &QPushButton::clicked, this, &QWidget::close);       // реф. SlotToBtnOk (persist+close)
    connect(btnCancel, &QPushButton::clicked, this, &QWidget::close);   // реф. SlotToBtnCancel→close

    // Реф. RefreshBtnEnable: Modify/Clone/Delete активны только при выделении в дереве.
    btnMod->setEnabled(false); btnCln->setEnabled(false); btnDel->setEnabled(false);
    connect(tree, &QTreeWidget::itemSelectionChanged, this, [=]() {
        bool has = !tree->selectedItems().isEmpty();
        btnMod->setEnabled(has); btnCln->setEnabled(has); btnDel->setEnabled(has);
    });
    // Add/Modify/Clone/Delete/SwitchDeviceType → KThesaurusOpt CRUD — DEVICE, не подключаем.
}
