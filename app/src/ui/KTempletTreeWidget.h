#pragma once

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QString>
#include <functional>

// Узел дерева шаблонов отчёта (реф. KTempletTreeWidgetItem : QTreeWidgetItem, size 0x168).
// Несёт payload KReportTemplateItem (в порте упрощён до id-строки + заголовок) + чекбокс
// (CheckStateRole на col0, flags Selectable|UserCheckable|Enabled).
class KTempletTreeWidgetItem : public QTreeWidgetItem
{
public:
    explicit KTempletTreeWidgetItem(const QString &title, const QString &id = QString());
    QString TemplateId() const { return m_id; }

private:
    QString m_id;   // реф. KReportTemplateItem.id (+0x40)
};

// Чекбокс-дерево шаблонов отчёта (реф. KTempletTreeWidget @ctor 0x560a28, base QTreeWidget +
// KObject — KObject-шина опущена как device). UI-порт. Категории (top-level) → шаблоны
// (дети), выбор = отметка чекбокса листа. InitLayout: 4 колонки (col0 stretch title+чекбокс,
// col1/2/3 fixed 20/30/30), header скрыт, scrollbar always-on, expandsOnDoubleClick off.
// Три-состояние: отметка родителя → все дети; смена ребёнка → пересчёт родителя (all/none/
// mixed). Наружу — SigClickItem(id) при отметке листа. DEVICE-STUB: данные из
// KReportTemplateManager/KTemplateLibCfg → инъектируемый SetTreeProvider.
class KTempletTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    explicit KTempletTreeWidget(QWidget *parent = nullptr);

    void InitLayout();     // реф. @0x55edb0
    void SetTreeProvider(std::function<void(KTempletTreeWidget *)> fn);   // DEVICE-STUB наполнение

signals:
    void SigClickItem(const QString &templateId);   // реф.: лист отмечен (выбор/применение)

private slots:
    void OnItemChanged(QTreeWidgetItem *item, int col);   // реф. OnItemChanged/OnItemCheckd

private:
    void updateChildren(QTreeWidgetItem *item, Qt::CheckState st);
    void updateParent(QTreeWidgetItem *item);

    bool m_guard = false;   // антирекурсия (реф. DisConnection/InitConnection)
    std::function<void(KTempletTreeWidget *)> m_treeProvider;
};
