#pragma once

#include <QAbstractTableModel>
#include <QTableView>
#include <QStyledItemDelegate>
#include <QList>
#include <QString>

// MVC-триплет таблицы DICOM-сервисов (реф. KDICOMSrvList{Model,View,Delegate}, хост KSysDicom).
// 6 колонок: [☑ вкл] Тип | Имя | AE Title | SID | Порт. Колонка 0 — чекбокс (реф. делегат рисует
// checkbox_checked/unchecked.png). Источник строк реф. = KDICOMConf (device) → в порте setRows-стаб.

struct DicomSrvRow {
    bool enabled = false;
    int type = 0;   // 0=Storage/1=Commitment/2=Worklist/3=MPPS
    QString name, aeTitle, station;
    int port = 104;
};

// Модель (реф. KDICOMSrvListModel : QAbstractTableModel, size 0x28). 6 колонок; data по колонкам;
// TextAlignment=center; UserRole → enabled (для делегата).
class KDICOMSrvListModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit KDICOMSrvListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation o, int role) const override;

    void setRows(const QList<DicomSrvRow> &rows);   // реф. LoadDICOMConf — seam
    void toggleEnabled(int row);

private:
    QList<DicomSrvRow> m_rows;
};

// Делегат (реф. KDICOMSrvListDelegate : QStyledItemDelegate). paint: колонка 0 — базовый + чекбокс-
// пиксмап (по enabled); колонки 1-5 — обычный текст.
class KDICOMSrvListDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit KDICOMSrvListDelegate(QObject *parent = nullptr);
    void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const override;
};

// Вью (реф. KDICOMSrvListView : QTableView). Колонки stretch, vheader скрыт, SelectRows/
// SingleSelection/NoEdit; клик по колонке 0 инвертирует enabled.
class KDICOMSrvListView : public QTableView
{
    Q_OBJECT
public:
    explicit KDICOMSrvListView(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *e) override;
};
