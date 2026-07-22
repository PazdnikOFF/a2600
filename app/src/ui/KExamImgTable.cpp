#include "KExamImgTable.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QHeaderView>

// ── KExamImgModel ────────────────────────────────────────────────────────────
KExamImgModel::KExamImgModel(QObject *parent)
    : KImgTableModel(1, 10, parent)   // реф. InitExamImgModel(1, 10): 1 ряд × 10 колонок
{
}

// ── KExamImgDelegate ─────────────────────────────────────────────────────────
KExamImgDelegate::KExamImgDelegate(int imgW, int imgH, QObject *parent)
    : KImgTableDelegate(imgW, imgH, parent)
    , m_imgW(imgW), m_imgH(imgH)
{
}

void KExamImgDelegate::paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const
{
    // Реф. KExamImgDelegate::paint @0x4757.. : тумбнейл по центру + order-num-бейдж + рамка.
    const QRect cell = opt.rect;
    const int imgX = cell.x() + cell.width() / 2 - m_imgW / 2;
    const int imgY = cell.y() + cell.height() / 2 - m_imgH / 2;
    const QRect imgRect(imgX, imgY, m_imgW, m_imgH);
    const qreal r = 8.0;   // реф. IMG_CORNER_RADIUS

    KImgTableItem *item = idx.data(Qt::DecorationRole).value<KImgTableItem *>();

    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);

    // Тумбнейл (скруглённый клип).
    if (item && !item->Image().isNull()) {
        QImage img = item->Image().scaled(m_imgW, m_imgH, Qt::KeepAspectRatio, Qt::FastTransformation);
        const int dx = imgX + (m_imgW - img.width()) / 2;
        const int dy = imgY + (m_imgH - img.height()) / 2;
        QPainterPath clip;
        clip.addRoundedRect(QRectF(dx, dy, img.width(), img.height()), r, r);
        p->setClipPath(clip);
        p->drawImage(dx, dy, img);
        p->setClipping(false);
    }

    // Рамка состояния (реф. DrawHighlightBorder, width 2): blue выбран / cyan hover / lightGray.
    QColor border = QColor(Qt::lightGray);          // 6
    if (opt.state & QStyle::State_Selected) border = QColor(Qt::blue);   // 9
    else if (opt.state & QStyle::State_MouseOver) border = QColor(Qt::cyan);   // 10
    QPainterPath bpath;
    bpath.addRoundedRect(QRectF(imgRect).adjusted(-2, -2, 2, 2), r, r);
    p->setPen(QPen(border, 2));
    p->setBrush(Qt::NoBrush);
    p->drawPath(bpath);

    // Order-num-бейдж (реф. DrawOrderNum): top-right 30×20, darkGray, белый Arial-12 «N+1».
    if (item) {
        const int bw = 30, bh = 20, m = 2;
        const QRect badge(imgX + m_imgW - bw - m, imgY + m, bw, bh);
        p->setPen(Qt::NoPen);
        p->setBrush(QColor(Qt::darkGray));   // 4 (cyan=10 в add-mark режиме — device-флаг, опущен)
        p->drawRoundedRect(badge, 8, 8);
        QFont f(QStringLiteral("Arial"), 12);
        p->setFont(f);
        p->setPen(QColor(Qt::white));        // 3
        p->drawText(badge.adjusted(0, -2, 0, -2), Qt::AlignCenter,
                    QString::number(item->GetOrderNum() + 1));
        // Edited-mark (реф. bottom-right) — глиф-точка вместо device-иконки edited_mark.png.
        if (item->GetEdited()) {
            p->setBrush(QColor(Qt::cyan));
            p->drawEllipse(QPoint(imgX + m_imgW - 8, imgY + m_imgH - 8), 4, 4);
        }
    }
    p->restore();
}

// ── KExamImgView ─────────────────────────────────────────────────────────────
KExamImgView::KExamImgView(QWidget *parent)
    : KImgTableView(parent)
{
    // Реф. InitExamImgView(159,130,130,110) + host: фикс-высота 130, скрытые заголовки.
    InitTableView(159, 130, 130, 110);
    setFixedHeight(130);
    setContentsMargins(0, 0, 0, 0);
}

void KExamImgView::OnItemActivated(const QModelIndex &idx)
{
    if (!idx.isValid())
        return;
    KImgTableItem *item = idx.data(Qt::DecorationRole).value<KImgTableItem *>();
    // Реф.: page==2 → SetSingleEditing + RequestSwitchImgOfAddMark; page==1 → удаление +
    // RequestUpdateUnusedImg. Режим-флаг g_eReportEditCurrentPage — device; эмитим оба сигнала.
    if (item)
        emit RequestSwitchImgOfAddMark(item);
    emit RequestUpdateUnusedImg();
}

void KExamImgView::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
        OnItemActivated(indexAt(e->pos()));
    KImgTableView::mousePressEvent(e);
}

void KExamImgView::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
        OnItemActivated(indexAt(e->pos()));
    KImgTableView::mouseDoubleClickEvent(e);
}

// ══ KUnusedImg триплет (= KExamImg + filename; сетка 3×7; одиночный клик-инверсия) ══
KUnusedImgModel::KUnusedImgModel(QObject *parent)
    : KImgTableModel(3, 7, parent)   // реф. InitUnusedImgModel(3, 7)
{
}

KUnusedImgDelegate::KUnusedImgDelegate(int imgW, int imgH, QObject *parent)
    : KExamImgDelegate(imgW, imgH, parent)
{
}

void KUnusedImgDelegate::paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const
{
    // Реф.: = KExamImg-рендер + filename под тумбнейлом.
    KExamImgDelegate::paint(p, opt, idx);
    KImgTableItem *item = idx.data(Qt::DecorationRole).value<KImgTableItem *>();
    if (item && !item->GetFileName().isEmpty()) {
        p->save();
        p->setPen(QColor(Qt::lightGray));
        QFont f = p->font(); f.setPointSize(9); p->setFont(f);
        QRect tr(opt.rect.x(), opt.rect.bottom() - 16, opt.rect.width(), 14);
        p->drawText(tr, Qt::AlignCenter, item->GetFileName());
        p->restore();
    }
}

KUnusedImgView::KUnusedImgView(QWidget *parent)
    : KImgTableView(parent)
{
    InitTableView(159, 148, 130, 110);   // +высота под filename
}

void KUnusedImgView::mousePressEvent(QMouseEvent *e)
{
    // Реф.: одиночный клик инвертирует выбор + сигнал переноса в used.
    if (e->button() == Qt::LeftButton) {
        const QModelIndex idx = indexAt(e->pos());
        if (idx.isValid()) {
            KImgTableItem *item = idx.data(Qt::DecorationRole).value<KImgTableItem *>();
            if (item) {
                item->SetIsSelected(!item->GetIsSelected());
                emit RequestUpdateSelectedImgFromUnusedView();
                viewport()->update();
            }
        }
    }
    KImgTableView::mousePressEvent(e);
}

// ══ KExamDetail триплет (чекбокс+filename+type-aware; двойной клик) ══
KExamDetailModel::KExamDetailModel(int rows, int cols, QObject *parent)
    : KImgTableModel(rows, cols, parent)
{
}

KExamDetailDelegate::KExamDetailDelegate(int imgW, int imgH, QObject *parent)
    : KImgTableDelegate(imgW, imgH, parent)
    , m_imgW(imgW), m_imgH(imgH)
{
}

void KExamDetailDelegate::paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const
{
    // Реф. KExamDetailDelegate::paint: тумбнейл + ЧЕКБОКС (top-right, вместо order-num) + filename
    // + edited + рамка. Type-aware: 0=image/1=video/2=report(TR_Rprt).
    const QRect cell = opt.rect;
    const int imgX = cell.x() + cell.width() / 2 - m_imgW / 2;
    const int imgY = cell.y() + 4;
    const QRect imgRect(imgX, imgY, m_imgW, m_imgH);
    const qreal r = 8.0;
    KImgTableItem *item = idx.data(Qt::DecorationRole).value<KImgTableItem *>();

    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    if (item && !item->Image().isNull()) {
        QImage img = item->Image().scaled(m_imgW, m_imgH, Qt::KeepAspectRatio, Qt::FastTransformation);
        const int dx = imgX + (m_imgW - img.width()) / 2, dy = imgY + (m_imgH - img.height()) / 2;
        QPainterPath clip; clip.addRoundedRect(QRectF(dx, dy, img.width(), img.height()), r, r);
        p->setClipPath(clip); p->drawImage(dx, dy, img); p->setClipping(false);
    }
    // Рамка состояния (как KExamImg).
    QColor border = QColor(Qt::lightGray);
    if (opt.state & QStyle::State_Selected) border = QColor(Qt::blue);
    else if (opt.state & QStyle::State_MouseOver) border = QColor(Qt::cyan);
    QPainterPath bpath; bpath.addRoundedRect(QRectF(imgRect).adjusted(-2, -2, 2, 2), r, r);
    p->setPen(QPen(border, 2)); p->setBrush(Qt::NoBrush); p->drawPath(bpath);
    if (item) {
        // Чекбокс top-right (реф. DrawCheckBox — выбран/нет).
        const QRect cb(imgX + m_imgW - 22, imgY + 4, 16, 16);
        p->setPen(QPen(QColor(Qt::white), 2));
        p->setBrush(item->GetIsSelected() ? QColor(Qt::cyan) : QColor(0, 0, 0, 120));
        p->drawRoundedRect(cb, 3, 3);
        if (item->GetIsSelected()) {   // галочка
            p->setPen(QPen(QColor(Qt::black), 2));
            p->drawLine(cb.left() + 3, cb.center().y(), cb.center().x(), cb.bottom() - 3);
            p->drawLine(cb.center().x(), cb.bottom() - 3, cb.right() - 2, cb.top() + 3);
        }
        // Filename / тип-подпись под тумбнейлом.
        QString label = item->GetFileName();
        if (item->GetType() == 2) label = QObject::tr("TR_Rprt");
        p->setPen(QColor(Qt::lightGray));
        QFont f = p->font(); f.setPointSize(9); p->setFont(f);
        p->drawText(QRect(cell.x(), imgY + m_imgH + 2, cell.width(), 14), Qt::AlignCenter, label);
    }
    p->restore();
}

KExamDetailView::KExamDetailView(int rows, int cols, QWidget *parent)
    : KImgTableView(parent)
{
    Q_UNUSED(rows); Q_UNUSED(cols);
    InitTableView(159, 148, 130, 110);
    setObjectName(QStringLiteral("exam_detail_view"));
}

void KExamDetailView::mouseDoubleClickEvent(QMouseEvent *e)
{
    // Реф.: инвертировать выбор + SelectedChanged + type-диспетч (в порте — только сигнал).
    if (e->button() == Qt::LeftButton) {
        const QModelIndex idx = indexAt(e->pos());
        if (idx.isValid()) {
            KImgTableItem *item = idx.data(Qt::DecorationRole).value<KImgTableItem *>();
            if (item) { item->SetIsSelected(!item->GetIsSelected()); emit SelectedChanged(); viewport()->update(); }
        }
    }
    KImgTableView::mouseDoubleClickEvent(e);
}
