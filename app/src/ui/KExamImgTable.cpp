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
