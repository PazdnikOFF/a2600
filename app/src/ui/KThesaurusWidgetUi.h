#pragma once

#include <QWidget>
#include <QStringList>
#include <functional>

class QLabel;
class QToolButton;
class QFrame;
class QComboBox;
class QTreeWidget;
class QTreeWidgetItem;

// Пикер глоссария/фраз для правки отчёта (реф. KThesaurusWidgetUi @ctor 0x4e57b0, base
// QWidget, 330×647, абс. геометрия). UI-порт. Пассивный: cmb_checkitem (тип эндоскопа) гонит
// tree_model (группы болезней→фразы); tbtn_glry_mng «...» открывает диалог управления
// (KThsaurusManageMentUi, уже портирован). Вставка — pull-based: хост цепляет tree и зовёт
// GetThesaurus(currentItem) → QStringList фраз (сигнала вставки НЕТ). DEVICE-STUB: наполнение
// дерева и фразы — KThesaurusOpt (per-scope файлы) → инъектируемый провайдер.
class KThesaurusWidgetUi : public QWidget
{
    Q_OBJECT
public:
    explicit KThesaurusWidgetUi(QWidget *parent = nullptr);

    void SetEndoScopeType(int scopeType);   // реф.: store + combo setCurrentIndex (→ rebuild)
    QTreeWidget *GetTreeWidget() const { return m_tree; }
    QStringList GetThesaurus(QTreeWidgetItem *item) const;   // реф.: фразы узла (DEVICE-STUB)

    // DEVICE-STUB инъекция: наполнить дерево по типу (реф. KThesaurusOpt::MakeTree).
    void SetTreeProvider(std::function<void(QTreeWidget *, int scopeType)> fn);
    void SetPhraseProvider(std::function<QStringList(const QString &key)> fn);

private slots:
    void SwitchEndoScopeType(int index);       // реф.: guard≤14, store, UpdateTreeWidget
    void SlotToOpenThesaurusMngDlg();          // реф.: OpenThsaurusManageMenDlg → refresh

private:
    void InitWidget();
    void UpdateTreeWidget();

    QLabel *m_labelGlry = nullptr;
    QToolButton *m_btnMng = nullptr;
    QFrame *m_frame = nullptr;
    QComboBox *m_cmbCheckItem = nullptr;
    QTreeWidget *m_tree = nullptr;
    int m_eScopeType = 0;
    std::function<void(QTreeWidget *, int)> m_treeProvider;
    std::function<QStringList(const QString &)> m_phraseProvider;
};
