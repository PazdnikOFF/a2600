#include "ui/KImgList.h"
#include "ui/KDisplayOption.h"

#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QDir>
#include <QPixmap>
#include <QIcon>

KImgList::KImgList(QWidget *parent) : QWidget(parent)
{
    // Размеры ячейки/иконки — из display-конфига (реф. GetKImgListCellRect/IconRect).
    const KDisplayOption &disp = KDisplayOption::Instance();
    QRect c = disp.GetKImgListCellRect();
    QRect i = disp.GetKImgListIconRect();
    cell_ = c.isValid() ? c.size() : QSize(190, 191);
    icon_ = i.isValid() ? i.size() : QSize(188, 135);
    initUiConfig();
}

void KImgList::initUiConfig()
{
    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);

    table_ = new QTableWidget(this);
    table_->setColumnCount(1);
    table_->horizontalHeader()->setVisible(false);
    table_->verticalHeader()->setVisible(false);
    table_->setShowGrid(false);
    table_->setSortingEnabled(false);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setIconSize(icon_);
    table_->setColumnWidth(0, cell_.width());
    table_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    table_->setStyleSheet("QTableWidget{background:transparent; border:none;}"
                          "QTableWidget::item{border:1px solid rgb(45,52,68);}");
    lay->addWidget(table_);
}

void KImgList::SetExamFolder(const QString &dir)
{
    examFolder_ = dir;
    AppendImg();
}

void KImgList::AppendImg()
{
    if (!table_) return;
    table_->setRowCount(0);
    // Миниатюры осмотра: <exam>/thumb/*.jpeg (SaveVideoThumbnail/CreateVideoSmallImage).
    QDir d(examFolder_ + "/thumb");
    const auto files = d.entryList({"*.jpeg", "*.jpg", "*.png"}, QDir::Files, QDir::Name);
    for (const QString &f : files) {
        const QString path = d.absoluteFilePath(f);
        auto *item = new QTableWidgetItem;
        item->setIcon(QIcon(QPixmap(path).scaled(icon_, Qt::KeepAspectRatio,
                                                 Qt::SmoothTransformation)));
        item->setData(Qt::UserRole, path);
        const int row = table_->rowCount();
        table_->insertRow(row);
        table_->setRowHeight(row, cell_.height());
        table_->setItem(row, 0, item);
    }
}

int KImgList::GetImgTotal() const
{
    return table_ ? table_->rowCount() : 0;
}

QString KImgList::GetCurrentImgPath() const
{
    if (!table_) return QString();
    auto *it = table_->currentItem();
    return it ? it->data(Qt::UserRole).toString() : QString();
}

void KImgList::PageNext()
{
    if (!table_ || table_->rowCount() == 0) return;
    const int r = qMin(table_->currentRow() + 1, table_->rowCount() - 1);
    table_->selectRow(r);
}

void KImgList::PagePre()
{
    if (!table_ || table_->rowCount() == 0) return;
    const int r = qMax(table_->currentRow() - 1, 0);
    table_->selectRow(r);
}
