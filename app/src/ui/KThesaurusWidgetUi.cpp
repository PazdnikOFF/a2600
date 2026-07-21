#include "KThesaurusWidgetUi.h"

#include <QLabel>
#include <QToolButton>
#include <QFrame>
#include <QComboBox>
#include <QTreeWidget>

KThesaurusWidgetUi::KThesaurusWidgetUi(QWidget *parent)
    : QWidget(parent)
{
    // Реф. ctor @0x4e57b0: дети (абс. геометрия) → InitWidget → InitConnect.
    setObjectName(QStringLiteral("KThesaurusWidgetUi"));
    resize(330, 647);

    m_labelGlry = new QLabel(tr("TR_Glry"), this);
    m_labelGlry->setObjectName(QStringLiteral("label_glry"));
    m_labelGlry->setGeometry(20, 8, 130, 25);

    m_btnMng = new QToolButton(this);
    m_btnMng->setObjectName(QStringLiteral("tbtn_glry_mng"));
    m_btnMng->setText(QStringLiteral("..."));
    m_btnMng->setGeometry(274, 8, 37, 25);

    m_frame = new QFrame(this);
    m_frame->setObjectName(QStringLiteral("frame"));
    m_frame->setFrameShape(QFrame::StyledPanel);
    m_frame->setFrameShadow(QFrame::Raised);
    m_frame->setGeometry(13, 38, 311, 601);

    m_cmbCheckItem = new QComboBox(m_frame);
    m_cmbCheckItem->setObjectName(QStringLiteral("cmb_checkitem"));
    m_cmbCheckItem->setGeometry(8, 11, 291, 31);

    m_tree = new QTreeWidget(m_frame);
    m_tree->setObjectName(QStringLiteral("tree_model"));
    m_tree->setGeometry(8, 51, 291, 541);
    m_tree->setColumnCount(1);
    m_tree->setHeaderHidden(true);   // реф. header-текст "1" — плейсхолдер (реально скрыт)

    InitWidget();

    // Реф. InitConnect.
    connect(m_cmbCheckItem, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &KThesaurusWidgetUi::SwitchEndoScopeType);
    connect(m_btnMng, &QToolButton::clicked, this, &KThesaurusWidgetUi::SlotToOpenThesaurusMngDlg);

    UpdateTreeWidget();
}

void KThesaurusWidgetUi::InitWidget()
{
    // Реф.: 5 типов эндоскопа + setCurrentIndex(m_eScopeType).
    for (const char *k : {"TR_Gspy", "TR_Cspy", "TR_Bspy", "TR_Nlspy", "TR_Cholngscpy"})
        m_cmbCheckItem->addItem(tr(k));
    m_cmbCheckItem->setCurrentIndex(m_eScopeType);
}

void KThesaurusWidgetUi::UpdateTreeWidget()
{
    // Реф.: scrollToTop + clear + KThesaurusOpt::MakeTree(scopeType) [DEVICE-STUB → провайдер].
    m_tree->scrollToTop();
    m_tree->clear();
    if (m_treeProvider)
        m_treeProvider(m_tree, m_eScopeType);
}

void KThesaurusWidgetUi::SwitchEndoScopeType(int index)
{
    // Реф.: guard ≤14, store, rebuild.
    if (index > 14 || index < 0)
        return;
    m_eScopeType = index;
    UpdateTreeWidget();
}

void KThesaurusWidgetUi::SetEndoScopeType(int scopeType)
{
    m_eScopeType = scopeType;
    m_cmbCheckItem->setCurrentIndex(scopeType);   // каскадно перестроит дерево
}

QStringList KThesaurusWidgetUi::GetThesaurus(QTreeWidgetItem *item) const
{
    // Реф.: лист + выбор → data(0,DisplayRole) ключ → KThesaurusOpt::GetDiseaseContentList
    // [DEVICE-STUB → провайдер].
    if (!item)
        return QStringList();
    const QString key = item->data(0, Qt::DisplayRole).toString();
    if (m_phraseProvider)
        return m_phraseProvider(key);
    return QStringList();
}

void KThesaurusWidgetUi::SlotToOpenThesaurusMngDlg()
{
    // Реф.: OpenThsaurusManageMenDlg(m_eScopeType) [открыл бы KThsaurusManageMentUi] → refresh.
    UpdateTreeWidget();
}

void KThesaurusWidgetUi::SetTreeProvider(std::function<void(QTreeWidget *, int)> fn)
{
    m_treeProvider = std::move(fn);
    UpdateTreeWidget();
}

void KThesaurusWidgetUi::SetPhraseProvider(std::function<QStringList(const QString &)> fn)
{
    m_phraseProvider = std::move(fn);
}
