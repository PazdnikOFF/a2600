#pragma once

#include <QTableView>
#include <QAbstractTableModel>
#include <QStyledItemDelegate>
#include <QString>
#include <QMap>
#include <QVector>
#include <QSet>

class QTimer;

// Определение колонки (реф. KHeaderProperty, 0x48 байт: title/key std::string + width/visible int).
struct KHeaderProperty {
    QString title;    // отображаемый заголовок (+0x00)
    QString key;      // ключ колонки в БД (+0x20)
    int width = 100;  // ширина (+0x40)
    bool visible = true;  // видимость (+0x44)
};

// Пейджинговая модель таблицы (реф. KTableModel : QAbstractTableModel). Ячейки — map
// ключ→значение по строке, кэш по странице. col 0 — чекбокс (UserCheckable). Все ячейки
// центрированы. При отсутствии страницы в кэше эмитит SigGetDataFromDB(page).
class KTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    KTableModel(const QVector<KHeaderProperty> &headers, const QString &mainKey,
                int rowsPerPage, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation o, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    // Реф.: наполнение страницы данными (page → строки).
    void SetModelData(int page, const QVector<QMap<QString, QString>> &rows);
    QString GetIndexMainKeyVaule(int row) const;
    int GetCheckedItemNumber() const { return m_checked.size(); }
    const QVector<KHeaderProperty> &Headers() const { return m_headers; }

signals:
    void SigGetDataFromDB(int page) const;
    void SigHeaderCheckBoxStateChange();

private:
    QVector<KHeaderProperty> m_headers;   // включая скрытые
    QVector<int> m_visibleCols;           // индексы видимых колонок
    QString m_mainKey;
    int m_rowsPerPage;
    mutable QMap<int, QVector<QMap<QString, QString>>> m_pageCache;   // page → строки
    QSet<QString> m_checked;              // отмеченные mainKey
};

// Делегат: рисует чекбокс в col 0 (реф. KLineDelegate : QStyledItemDelegate). Фон ячейки
// #1a1a1a + один из 4 PNG (checked/unchecked × normal/hover). Клик по col 0 — тоггл;
// одиночный/двойной клик по прочим — через QTimer.
class KLineDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit KLineDelegate(QObject *parent = nullptr);
    void SetDelegateView(QAbstractItemView *v) { m_view = v; }
    void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const override;
    bool editorEvent(QEvent *e, QAbstractItemModel *model,
                     const QStyleOptionViewItem &opt, const QModelIndex &idx) override;

signals:
    void SigToDoubleClick(const QModelIndex &index);
    void SigToHighlight(const QModelIndex &index);
    void SigToSpaceKeyPress(const QModelIndex &index);

private:
    QString checkboxAsset(const QString &name) const;   // ProjectPresetPath + patient/checkbox/
    QAbstractItemView *m_view = nullptr;   // +0x30
};

// Кастомная таблица (реф. KTableView : QTableView, ctor @0x7bc278). Ставит KTableModel +
// KLineDelegate в InitTableView (не в ctor). Целая строка — выделение (selectRow),
// SingleSelection, NoEditTriggers, скрытый вертикальный заголовок, sort-индикатор.
class KTableView : public QTableView
{
    Q_OBJECT
public:
    explicit KTableView(QWidget *parent = nullptr);

    void InitTableView(const QVector<KHeaderProperty> &headers, const QString &mainKey,
                       int rowsPerPage, int defaultSelCol);   // реф. @0x7bc990
    KTableModel *GetModel() const { return m_model; }
    int GetCurrentRowIdx() const { return currentIndex().row(); }

signals:
    void SigGetDataFromDB(int page);
    void SigHeaderCheckBoxStateChange();
    void SigToDoubleClick(const QModelIndex &index);
    void SigToFocusOutCurrentView();
    void SigToKeyPressF2Event();
    void SigToKeyPressEnterEvent();

protected:
    void keyPressEvent(QKeyEvent *) override;

private:
    void SetTableColumnWidth(const QVector<KHeaderProperty> &headers);
    void SetTableColumnHidden(const QVector<KHeaderProperty> &headers);
    void SetTableViewSignalConnect();

    QVector<KHeaderProperty> m_headers;   // +0x30
    KTableModel *m_model = nullptr;       // +0x48
    KLineDelegate *m_delegate = nullptr;  // +0x50
    int m_defaultSelCol = -1;             // +0x58
};
