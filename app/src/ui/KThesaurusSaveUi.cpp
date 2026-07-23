#include "KThesaurusSaveUi.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QWidget>

KThesaurusSaveUi::KThesaurusSaveUi(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x4e3e30: KDialog(modal=false) → new KThesaurusOpt (device) → setupUi →
    // префилл текстов/scope → InitWidget (maxLen100, ReadFile→cmb_group, SetTitle) → InitConnect.
    setupUi();
    SetTitle(tr("TR_AGlossary"));   // реф. InitWidget перекрывает setupUi-титул TR_Glry
}

void KThesaurusSaveUi::setupUi()
{
    setObjectName(QStringLiteral("KThesaurusSaveUi"));

    QWidget *host = ContentArea();
    host->setFixedSize(1024, 768);   // реф. resize(1024,768), абсолютная геометрия

    // === Блок «группа / заголовок» ===
    QWidget *wGrp = new QWidget(host);
    wGrp->setObjectName(QStringLiteral("widget_diseagroup"));
    wGrp->setGeometry(28, 54, 972, 125);
    QComboBox *cmbGroup = new QComboBox(wGrp);   // реф. device-список (GetDiseagroupList)
    cmbGroup->setObjectName(QStringLiteral("cmb_group"));
    cmbGroup->setGeometry(163, 19, 778, 31);
    cmbGroup->setEditable(true);
    QLineEdit *edTitle = new QLineEdit(wGrp);
    edTitle->setObjectName(QStringLiteral("edit_title"));
    edTitle->setGeometry(163, 68, 778, 35);
    edTitle->setMaxLength(100);                  // реф. InitWidget setMaxLength(100)
    QLabel *lGrp = new QLabel(tr("TR_Grp:"), wGrp);
    lGrp->setObjectName(QStringLiteral("label_group"));
    lGrp->setGeometry(40, 24, 97, 21);
    QLabel *lTtl = new QLabel(tr("TR_Ttle:"), wGrp);
    lTtl->setObjectName(QStringLiteral("label_title"));
    lTtl->setGeometry(40, 75, 97, 21);
    QLabel *starG = new QLabel(QStringLiteral("*"), wGrp);   // реф. обязательное поле
    starG->setObjectName(QStringLiteral("label_group_star"));
    starG->setGeometry(32, 24, 6, 21);
    starG->setStyleSheet(QStringLiteral("color:#e05050;"));
    QLabel *starT = new QLabel(QStringLiteral("*"), wGrp);
    starT->setObjectName(QStringLiteral("label_title_star"));
    starT->setGeometry(32, 75, 6, 21);
    starT->setStyleSheet(QStringLiteral("color:#e05050;"));

    // === Блок «находки / заключение» ===
    QWidget *wDiag = new QWidget(host);
    wDiag->setObjectName(QStringLiteral("widget_diag"));
    wDiag->setGeometry(28, 189, 972, 460);
    QLabel *lFind = new QLabel(tr("TR_VOExam:"), wDiag);
    lFind->setObjectName(QStringLiteral("label_examfinding"));
    lFind->setGeometry(36, 15, 97, 25);
    QLabel *lDiag = new QLabel(tr("TR_EConclusion:"), wDiag);
    lDiag->setObjectName(QStringLiteral("label_diagresult"));
    lDiag->setGeometry(34, 283, 97, 25);
    QTextEdit *edFind = new QTextEdit(wDiag);   // реф. префилл examFinding (ctor-арг)
    edFind->setObjectName(QStringLiteral("edit_examfinding"));
    edFind->setGeometry(32, 46, 909, 218);
    edFind->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    QTextEdit *edDiag = new QTextEdit(wDiag);   // реф. префилл diagResult (ctor-арг)
    edDiag->setObjectName(QStringLiteral("edit_diagresult"));
    edDiag->setGeometry(32, 314, 909, 127);
    edDiag->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    // === Подсказка + кнопки ===
    QLabel *tip = new QLabel(tr("TR_RField"), host);
    tip->setObjectName(QStringLiteral("label_input_tip"));
    tip->setGeometry(313, 666, 402, 25);
    tip->setAlignment(Qt::AlignCenter);
    QPushButton *btnSave = new QPushButton(tr("TR_Sve"), host);   // реф. SlotBtnSave (device)
    btnSave->setObjectName(QStringLiteral("btn_save"));
    btnSave->setGeometry(374, 703, 133, 40);
    QPushButton *btnCancel = new QPushButton(tr("TR_Ccl"), host);
    btnCancel->setObjectName(QStringLiteral("btn_cancel"));
    btnCancel->setGeometry(546, 703, 133, 40);
    connect(btnCancel, &QPushButton::clicked, this, &QWidget::close);   // реф. SlotBtnCancel→close
}

void KThesaurusSaveUi::SetSaveContext(const QString &text, const QString &title, int scopeClass)
{
    m_text = text;
    m_title = title;
    m_scopeClass = scopeClass;
}
