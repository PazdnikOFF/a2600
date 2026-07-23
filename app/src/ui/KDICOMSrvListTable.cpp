#include "KDICOMSrvListTable.h"

#include <algorithm>
#include "Theme.h"

#include <QHeaderView>
#include <QPainter>
#include <QMouseEvent>
#include <QPixmap>

// ── KDICOMSrvListModel ───────────────────────────────────────────────────────
KDICOMSrvListModel::KDICOMSrvListModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int KDICOMSrvListModel::rowCount(const QModelIndex &) const { return m_rows.size(); }
int KDICOMSrvListModel::columnCount(const QModelIndex &) const { return 6; }   // реф. 6 колонок

QVariant KDICOMSrvListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return QVariant();
    const DicomSrvRow &r = m_rows[index.row()];
    if (role == Qt::TextAlignmentRole)
        return int(Qt::AlignCenter);   // реф. 0x84
    if (role == Qt::UserRole)
        return r.enabled;              // для делегата (чекбокс)
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0: return QString();      // чекбокс-колонка (пусто)
        case 1:                        // реф. GetDICOMServiceConfInfoList тип-строка
            switch (r.type) {
            case 0: return tr("TR_Strge");
            case 1: return tr("TR_SCommitment");
            case 2: return tr("TR_Wrklst");
            case 3: return tr("TR_MPPS");
            default: return QString();
            }
        case 2: return r.name;
        case 3: return r.aeTitle;
        case 4: return r.station;
        case 5: return QString::number(r.port);
        }
    }
    return QVariant();
}

QVariant KDICOMSrvListModel::headerData(int section, Qt::Orientation o, int role) const
{
    // Реф. InitHeaderData: 6 tr-ключей.
    if (o == Qt::Horizontal && role == Qt::DisplayRole) {
        static const char *const keys[6] = {
            "TR_Susing", "TR_SType", "TR_SName", "TR_ATitle", "TR_SIDName", "TR_Prt"
        };
        if (section >= 0 && section < 6)
            return tr(keys[section]);
    }
    return QVariant();
}

void KDICOMSrvListModel::SortDicomItem()
{
    // Реф. @0x5b8d60: разложить по двум векторам, отсортировать каждый компаратором
    // `*a > *b` (то есть по addTime убыв.), затем склеить A + B.
    QList<DicomSrvRow> a, b;
    for (const DicomSrvRow &r : qAsConst(m_rows)) {
        if (r.type != 1 && r.enabled)
            a.append(r);
        else
            b.append(r);
    }
    auto newerFirst = [](const DicomSrvRow &x, const DicomSrvRow &y) {
        return y.addTime < x.addTime;   // реф. operator>: rhs.time < this->time
    };
    std::stable_sort(a.begin(), a.end(), newerFirst);
    std::stable_sort(b.begin(), b.end(), newerFirst);
    beginResetModel();
    m_rows = a + b;
    endResetModel();
}

void KDICOMSrvListModel::setRows(const QList<DicomSrvRow> &rows)
{
    beginResetModel();
    m_rows = rows;
    endResetModel();
}

void KDICOMSrvListModel::toggleEnabled(int row)
{
    if (row >= 0 && row < m_rows.size()) {
        m_rows[row].enabled = !m_rows[row].enabled;
        emit dataChanged(index(row, 0), index(row, 0));
    }
}

// ── KDICOMSrvListDelegate ────────────────────────────────────────────────────
KDICOMSrvListDelegate::KDICOMSrvListDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void KDICOMSrvListDelegate::paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const
{
    QStyledItemDelegate::paint(p, opt, idx);
    if (idx.column() != 0)
        return;
    // Реф.: колонка 0 — чекбокс-пиксмап (checked/unchecked) по enabled.
    const bool enabled = idx.data(Qt::UserRole).toBool();
    const QString name = enabled ? QStringLiteral("checkbox_checked.png")
                                 : QStringLiteral("checkbox_unchecked.png");
    QPixmap pm(theme::asset(QStringLiteral("black/icon/checkbox/new/") + name));
    if (pm.isNull())
        return;
    const QSize sz(18, 18);
    const QRect r = opt.rect;
    const QPoint pos(r.x() + (r.width() - sz.width()) / 2, r.y() + (r.height() - sz.height()) / 2);
    p->drawPixmap(QRect(pos, sz), pm);
}

// ── KDICOMSrvListView ────────────────────────────────────────────────────────
KDICOMSrvListView::KDICOMSrvListView(QWidget *parent)
    : QTableView(parent)
{
    // Реф. ctor: колонки stretch, vheader скрыт, SelectRows/SingleSelection/NoEdit.
    horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    verticalHeader()->setVisible(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void KDICOMSrvListView::mousePressEvent(QMouseEvent *e)
{
    // Реф.: клик по колонке 0 инвертирует enabled сервиса.
    const QModelIndex idx = indexAt(e->pos());
    if (idx.isValid() && idx.column() == 0) {
        if (auto *m = qobject_cast<KDICOMSrvListModel *>(model()))
            m->toggleEnabled(idx.row());
    }
    QTableView::mousePressEvent(e);
}
