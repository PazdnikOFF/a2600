#include "KReportTempletEditDlg.h"

#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWidget>

KReportTempletEditDlg::KReportTempletEditDlg(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x51f300: KFullScreenDialog(parent,-1) → setupUi → InitTemplateLib/
    // InitTemplate (device комбо) → InitWidget (SetTitle(TR_Tpte) + override текстов) →
    // InitStyleSheet → InitConnection.
    setupUi();
    SetKStyle(KDLG_FULLSCREEN);        // реф. полноэкранная страница 1920×1080
    SetTitle(tr("TR_Tpte"));
}

void KReportTempletEditDlg::setupUi()
{
    setObjectName(QStringLiteral("KReportTempletEditDlg"));

    QWidget *host = ContentArea();
    QVBoxLayout *root = new QVBoxLayout(host);
    root->setObjectName(QStringLiteral("verticalLayout"));
    root->setContentsMargins(9, 45, 9, 9);

    // ===================== Основной ряд: левая колонка | правый предпросмотр =====================
    QHBoxLayout *hMain = new QHBoxLayout();
    hMain->setObjectName(QStringLiteral("horizontalLayout_main"));
    hMain->setContentsMargins(22, 10, 0, 0);

    QVBoxLayout *vLeft = new QVBoxLayout();
    vLeft->setObjectName(QStringLiteral("verticalLayout_main_left"));

    // Верх левой колонки: подпись + комбо библиотеки шаблонов (device).
    QWidget *leftTop = new QWidget(host);
    leftTop->setObjectName(QStringLiteral("left_top_widget"));
    QHBoxLayout *hTop = new QHBoxLayout(leftTop);
    QLabel *lTpl = new QLabel(tr("TR_Tpte:"), leftTop);
    lTpl->setObjectName(QStringLiteral("label_templet"));
    lTpl->setMinimumWidth(161);
    hTop->addWidget(lTpl);
    QComboBox *cmbTpl = new QComboBox(leftTop);   // реф. device: GetTempletsInfos
    cmbTpl->setObjectName(QStringLiteral("comb_templet"));
    cmbTpl->setMaximumWidth(280);
    hTop->addWidget(cmbTpl);
    hTop->addStretch(1);
    vLeft->addWidget(leftTop);

    // Низ левой колонки: дерево секций (KTempletTreeWidget → QTreeWidget).
    QWidget *leftBottom = new QWidget(host);
    leftBottom->setObjectName(QStringLiteral("left_bottom_widget"));
    QGridLayout *g2 = new QGridLayout(leftBottom);
    QTreeWidget *tree = new QTreeWidget(leftBottom);
    tree->setObjectName(QStringLiteral("tree_content"));
    tree->setColumnCount(1);
    tree->setHeaderHidden(true);                          // реф. setHeaderHidden(true)
    tree->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tree->header()->setStretchLastSection(true);
    g2->addWidget(tree, 0, 0);
    vLeft->addWidget(leftBottom, 1);

    hMain->addLayout(vLeft, 0);   // реф. stretch 0

    // Правый предпросмотр/редактор (KNewTempletEditor → read-only QTextEdit).
    QWidget *rightW = new QWidget(host);
    rightW->setObjectName(QStringLiteral("right_widget"));
    rightW->setMinimumWidth(1200);
    QGridLayout *gR = new QGridLayout(rightW);
    QTextEdit *edTpl = new QTextEdit(rightW);
    edTpl->setObjectName(QStringLiteral("txt_edit_templet"));
    edTpl->setMinimumSize(820, 820);   // реф. min=max=820 (квадрат)
    edTpl->setReadOnly(true);          // реф. read-only WYSIWYG-документ
    gR->addWidget(edTpl, 0, 0);
    hMain->addWidget(rightW, 1);   // реф. stretch 1

    root->addLayout(hMain, 1);

    // ===================== Ряд кнопок =====================
    QHBoxLayout *hBtn = new QHBoxLayout();
    hBtn->setObjectName(QStringLiteral("horizontalLayout_bottom_buttons"));
    hBtn->setContentsMargins(0, 6, 0, 6);
    auto mkBtn = [&](const char *name, const QString &text) {
        QPushButton *b = new QPushButton(text, host);
        b->setObjectName(QString::fromLatin1(name));
        b->setMinimumWidth(210);
        return b;
    };
    QPushButton *btnExit = mkBtn("btn_exit", tr("TR_CExit"));   // реф. InitWidget override (setupUi TR_Ext)
    QPushButton *btnSaveExit = mkBtn("btn_save_exit", tr("TR_SExit"));
    hBtn->addWidget(btnExit);
    hBtn->addWidget(btnSaveExit);
    hBtn->addStretch(1);
    QPushButton *btnEditInfo = mkBtn("btn_editInfo", tr("TR_EHInformation"));
    QPushButton *btnDefault = mkBtn("btn_default", tr("TR_Dflt"));   // реф. override (setupUi TR_RDefault)
    hBtn->addWidget(btnEditInfo);
    hBtn->addWidget(btnDefault);
    root->addLayout(hBtn);

    connect(btnExit, &QPushButton::clicked, this, &QWidget::close);   // реф. OnExit→close
    // Save&Exit/Default/EditInfo/OnDeptChanged/add-del-update — DEVICE, не подключаем.
}
