#include "KTableView.h"
#include "sys/KEnvConfig.h"

#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QHeaderView>
#include <QDir>
#include <QPixmap>
#include <QCursor>

// ── KTableModel ──────────────────────────────────────────────────────────────

KTableModel::KTableModel(const QVector<KHeaderProperty> &headers, const QString &mainKey,
                         int rowsPerPage, QObject *parent)
    : QAbstractTableModel(parent)
    , m_headers(headers)
    , m_mainKey(mainKey)
    , m_rowsPerPage(rowsPerPage > 0 ? rowsPerPage : 1)
{
    for (int i = 0; i < m_headers.size(); ++i)
        if (m_headers[i].visible)
            m_visibleCols.append(i);
}

int KTableModel::rowCount(const QModelIndex &) const { return m_rowsPerPage; }
int KTableModel::columnCount(const QModelIndex &) const { return m_visibleCols.size(); }

QVariant KTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.column() >= m_visibleCols.size())
        return QVariant();
    const int row = index.row();
    const int page = row / m_rowsPerPage;
    const int localRow = row % m_rowsPerPage;
    const KHeaderProperty &h = m_headers[m_visibleCols[index.column()]];

    if (role == Qt::TextAlignmentRole)
        return int(Qt::AlignHCenter | Qt::AlignVCenter);   // реф. 0x84

    if (!m_pageCache.contains(page)) {
        emit const_cast<KTableModel *>(this)->SigGetDataFromDB(page);   // ленивая подгрузка
        return QVariant();
    }
    const QVector<QMap<QString, QString>> &rows = m_pageCache[page];
    if (localRow >= rows.size())
        return QVariant();
    const QMap<QString, QString> &r = rows[localRow];

    if (role == Qt::CheckStateRole && index.column() == 0)
        return m_checked.contains(r.value(m_mainKey)) ? Qt::Checked : Qt::Unchecked;
    if (role == Qt::DisplayRole)
        return r.value(h.key);
    return QVariant();
}

QVariant KTableModel::headerData(int section, Qt::Orientation o, int role) const
{
    if (o == Qt::Horizontal && role == Qt::DisplayRole && section < m_visibleCols.size())
        return m_headers[m_visibleCols[section]].title;
    return QAbstractTableModel::headerData(section, o, role);
}

Qt::ItemFlags KTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    Qt::ItemFlags f = Qt::ItemIsSelectable | Qt::ItemIsEnabled;   // 0x21, НЕ editable
    if (index.column() == 0)
        f |= Qt::ItemIsUserCheckable;   // 0x31
    return f;
}

bool KTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::CheckStateRole || index.column() != 0 || !index.isValid())
        return false;
    const int page = index.row() / m_rowsPerPage;
    const int localRow = index.row() % m_rowsPerPage;
    if (!m_pageCache.contains(page) || localRow >= m_pageCache[page].size())
        return false;
    const QString key = m_pageCache[page][localRow].value(m_mainKey);
    if (value.toInt() == Qt::Checked)
        m_checked.insert(key);
    else
        m_checked.remove(key);
    emit dataChanged(index, index);
    emit SigHeaderCheckBoxStateChange();
    return true;
}

void KTableModel::SetModelData(int page, const QVector<QMap<QString, QString>> &rows)
{
    beginResetModel();
    m_pageCache[page] = rows;
    endResetModel();
}

QString KTableModel::GetIndexMainKeyVaule(int row) const
{
    const int page = row / m_rowsPerPage, localRow = row % m_rowsPerPage;
    if (!m_pageCache.contains(page) || localRow >= m_pageCache[page].size())
        return QString();
    return m_pageCache[page][localRow].value(m_mainKey);
}

// ── KLineDelegate ────────────────────────────────────────────────────────────

KLineDelegate::KLineDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QString KLineDelegate::checkboxAsset(const QString &name) const
{
    // Реф.: KSystem::ProjectPresetPath() + "patient/checkbox/<name>".
    return QDir(QString::fromStdString(KEnvConfig::GetInstance().GetReadOnlyBaseDir()))
        .absoluteFilePath(QStringLiteral("patient/checkbox/") + name);
}

void KLineDelegate::paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const
{
    if (idx.column() != 0) {
        QStyledItemDelegate::paint(p, opt, idx);
        return;
    }
    // Реф. col-0: фон #1a1a1a + один из 4 PNG по checked/hover.
    p->fillRect(opt.rect, QColor(26, 26, 26));
    const bool checked = idx.data(Qt::CheckStateRole).toInt() == Qt::Checked;
    bool hover = false;
    if (m_view)
        hover = opt.rect.contains(m_view->viewport()->mapFromGlobal(QCursor::pos()));
    QString name = checked ? QStringLiteral("checkbox_checked") : QStringLiteral("checkbox_unchecked");
    if (hover)
        name += QStringLiteral("_hover");
    name += QStringLiteral(".png");
    const QPixmap pm(checkboxAsset(name));
    if (!pm.isNull()) {
        const int x = opt.rect.left() + (opt.rect.width() - pm.width()) / 2 + 2;
        const int y = opt.rect.top() + (opt.rect.height() - pm.height()) / 2;
        p->drawPixmap(x, y, pm);
    }
}

bool KLineDelegate::editorEvent(QEvent *e, QAbstractItemModel *model,
                                const QStyleOptionViewItem &opt, const QModelIndex &idx)
{
    if (idx.column() == 0 && e->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *me = static_cast<QMouseEvent *>(e);
        if (me->button() == Qt::LeftButton && opt.rect.contains(me->pos())) {
            const bool checked = idx.data(Qt::CheckStateRole).toInt() == Qt::Checked;
            model->setData(idx, checked ? Qt::Unchecked : Qt::Checked, Qt::CheckStateRole);
            return true;
        }
    }
    if (e->type() == QEvent::MouseButtonDblClick) {
        emit SigToDoubleClick(idx);
        return true;
    }
    if (e->type() == QEvent::KeyPress && static_cast<QKeyEvent *>(e)->key() == Qt::Key_Space) {
        emit SigToSpaceKeyPress(idx);
        return true;
    }
    return QStyledItemDelegate::editorEvent(e, model, opt, idx);
}

// ── KTableView ───────────────────────────────────────────────────────────────

KTableView::KTableView(QWidget *parent)
    : QTableView(parent)
{
    // Реф. ctor @0x7bc278: eventFilter + WA_MouseTracking на viewport. Поведение выделения
    // в реф. задаётся императивно через selectRow — для порта ставим явно.
    viewport()->setAttribute(Qt::WA_MouseTracking, true);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    verticalHeader()->hide();
    horizontalHeader()->setSortIndicatorShown(true);
}

void KTableView::InitTableView(const QVector<KHeaderProperty> &headers, const QString &mainKey,
                               int rowsPerPage, int defaultSelCol)
{
    // Реф. @0x7bc990.
    m_headers = headers;
    m_model = new KTableModel(headers, mainKey, rowsPerPage, this);
    setModel(m_model);

    if (defaultSelCol >= 0) {
        m_delegate = new KLineDelegate(this);
        m_delegate->SetDelegateView(this);
        setItemDelegate(m_delegate);
        m_defaultSelCol = defaultSelCol;
    }
    SetTableColumnWidth(headers);
    SetTableColumnHidden(headers);
    horizontalHeader()->setSortIndicator(defaultSelCol, Qt::DescendingOrder);
    SetTableViewSignalConnect();
    resizeColumnsToContents();
    // Реф.: авто-подгон не должен превышать заданную ширину.
    int vis = 0;
    for (const KHeaderProperty &h : headers) {
        if (!h.visible) continue;
        if (columnWidth(vis) > h.width)
            setColumnWidth(vis, h.width);
        ++vis;
    }
}

void KTableView::SetTableColumnWidth(const QVector<KHeaderProperty> &headers)
{
    int vis = 0;
    for (const KHeaderProperty &h : headers) {
        if (!h.visible) continue;
        setColumnWidth(vis, h.width);
        ++vis;
    }
}

void KTableView::SetTableColumnHidden(const QVector<KHeaderProperty> &)
{
    // Скрытые колонки исключены из модели (columnCount по visible) — доп. скрытие не нужно.
}

void KTableView::SetTableViewSignalConnect()
{
    // Реф. @0x7bc450: model→view и delegate→view форварды.
    connect(m_model, &KTableModel::SigGetDataFromDB, this, &KTableView::SigGetDataFromDB);
    connect(m_model, &KTableModel::SigHeaderCheckBoxStateChange,
            this, &KTableView::SigHeaderCheckBoxStateChange);
    if (m_delegate) {
        connect(m_delegate, &KLineDelegate::SigToDoubleClick, this, &KTableView::SigToDoubleClick);
        connect(m_delegate, &KLineDelegate::SigToHighlight, this,
                [this](const QModelIndex &idx) { if (idx.isValid()) selectRow(idx.row()); });
    }
}

void KTableView::keyPressEvent(QKeyEvent *e)
{
    // Реф. @0x7bcb58.
    const int row = currentIndex().row();
    switch (e->key()) {
    case Qt::Key_Up:
        if (row > 0) selectRow(row - 1);
        return;
    case Qt::Key_Down:
        if (m_model && row < m_model->rowCount() - 1) selectRow(row + 1);
        return;
    case Qt::Key_Tab:
        emit SigToFocusOutCurrentView();
        return;
    case Qt::Key_F2:
        emit SigToKeyPressF2Event();
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        emit SigToKeyPressEnterEvent();
        return;
    default:
        QTableView::keyPressEvent(e);
    }
}
