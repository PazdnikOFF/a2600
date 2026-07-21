#include "KImgTableView.h"

#include <QHeaderView>
#include <QPainter>
#include <QtMath>

// ── KImgTableItem ────────────────────────────────────────────────────────────

KImgTableItem::KImgTableItem(QObject *parent)
    : QObject(parent)
{
}

void KImgTableItem::LoadImgToQImage()
{
    // Реф. LoadImgToQImage: ленивая загрузка тумбнейла с диска.
    if (m_img.isNull() && !m_thumbPath.isEmpty())
        m_img.load(m_thumbPath);
}

// ── KImgTableModel ───────────────────────────────────────────────────────────

KImgTableModel::KImgTableModel(int rows, int cols, QObject *parent)
    : QStandardItemModel(rows, cols, parent)
    , m_cols(cols > 0 ? cols : 1)
{
    qRegisterMetaType<KImgTableItem *>("KImgTableItem*");
}

void KImgTableModel::SetImages(const QVector<KImgTableItem *> &items)
{
    m_items = items;
    m_pageSize = 0;   // одна страница: pageSize не ограничивает
    const int rows = (m_items.size() + m_cols - 1) / m_cols;
    setRowCount(qMax(rows, 1));
    setColumnCount(m_cols);
}

QVariant KImgTableModel::data(const QModelIndex &index, int role) const
{
    // Реф. @0x4b6798: только DecorationRole → указатель KImgTableItem*.
    if (role != Qt::DecorationRole || !index.isValid())
        return QVariant();
    const int flat = m_cols * index.row() + (m_currentPage - 1) * m_pageSize + index.column();
    if (flat < 0 || flat >= m_items.size())
        return QVariant();
    return QVariant::fromValue(m_items[flat]);
}

// ── KImgTableDelegate ────────────────────────────────────────────────────────

KImgTableDelegate::KImgTableDelegate(int imgW, int imgH, QObject *parent)
    : QStyledItemDelegate(parent)
    , m_imgW(imgW)
    , m_imgH(imgH)
{
}

void KImgTableDelegate::paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const
{
    KImgTableItem *item = idx.data(Qt::DecorationRole).value<KImgTableItem *>();
    if (!item || item->isNullPath()) {
        p->eraseRect(opt.rect);
        QStyledItemDelegate::paint(p, opt, idx);
        return;
    }
    item->LoadImgToQImage();
    const QImage &img = item->Image();
    if (!img.isNull()) {
        const QImage scaled = img.scaled(m_imgW, m_imgH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        // Реф.: центр по X, чуть выше по Y (место под подпись/бейдж).
        const int x = opt.rect.left() + opt.rect.width() / 2 - scaled.width() / 2;
        const int y = opt.rect.top() + (opt.rect.height() / 2 - scaled.height() / 2) / 2;
        p->drawImage(x, y, scaled);
    }
    // Рамка выделения (реф. DrawHighlightBorder: selected→blue, hover→cyan, иначе lightGray).
    QColor border = Qt::lightGray;
    if (opt.state & QStyle::State_Selected) border = Qt::blue;
    else if (opt.state & QStyle::State_MouseOver) border = Qt::cyan;
    p->setPen(QPen(border, 2));
    p->setBrush(Qt::NoBrush);
    p->drawRect(opt.rect.adjusted(1, 1, -2, -2));
}

// ── KImgTableView ────────────────────────────────────────────────────────────

KImgTableView::KImgTableView(QWidget *parent)
    : QTableView(parent)
{
    // Реф. ctor @0x4b9ab0: тривиален (индексы -1). Конфиг — в InitTableView.
}

void KImgTableView::InitTableView(int cellW, int cellH, int imgW, int imgH)
{
    // Реф. @0x4b9b90.
    horizontalHeader()->setVisible(false);
    verticalHeader()->setVisible(false);
    horizontalHeader()->setDefaultSectionSize(cellW);
    verticalHeader()->setDefaultSectionSize(cellH);
    setShowGrid(false);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setAttribute(Qt::WA_Hover, true);

    m_model = new KImgTableModel(1, m_cols, this);
    setModel(m_model);
    m_delegate = new KImgTableDelegate(imgW, imgH, this);
    setItemDelegate(m_delegate);
}
