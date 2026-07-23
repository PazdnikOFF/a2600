#include "ui/KImgList.h"
#include "ui/KDisplayOption.h"
#include "ui/KImgListCell.h"
#include "ui/KTableWidget.h"

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
    // Дефолты — РЕФЕРЕНСНЫЕ (@0x6508a8/@0x650ac8): значение по умолчанию у QSettings::value
    // задано как QRect(0,0,269,209) и QRect(0,0,269,201) в координатной форме (x1,y1,x2,y2),
    // то есть размеры 270×210 и 270×202. Раньше тут стояли выдуманные 190×191 / 188×135.
    cell_ = c.isValid() ? c.size() : QSize(270, 210);
    icon_ = i.isValid() ? i.size() : QSize(270, 202);
    initUiConfig();
}

void KImgList::initUiConfig()
{
    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);

    // Реф. Ui_KImgList::setupUi @0x681018: таблица — KTableWidget (KlList_box), и настройки
    // ставит именно владелец (сам класс в ctor не настраивает ничего).
    table_ = new KTableWidget(this);
    table_->setObjectName(QStringLiteral("KlList_box"));
    table_->setTabKeyNavigation(false);                 // реф.
    table_->setTextElideMode(Qt::ElideMiddle);          // реф. 2
    table_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);   // реф. 1
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

    // Реф. Ui_KImgList::setupUi @0x681018: под таблицей — ЧЕТЫРЕ ячейки-превью
    // KlList_img0..3 (реальный класс KImgListCell, не подстановка).
    for (int n = 0; n < 4; ++n) {
        auto *cell = new KImgListCell(this);
        cell->setObjectName(QStringLiteral("KlList_img%1").arg(n));
        // Реф. KImgList::initImgInfo @0x67d448: размеры из icon-rect + снятие рамок.
        cell->setMinimumSize(icon_.width(), icon_.height());
        cell->setMaximumSize(icon_.width(), icon_.height());
        cell->setStyleSheet(QStringLiteral(
            "QFrame{border: 0px solid transparent;} QLabel{border: 0px solid transparent;}"));
        m_cells[n] = cell;
        lay->addWidget(cell);
    }
}

void KImgList::SetImgInfo(int index, const QString &imgPath)
{
    // Реф. @0x67fe38: пустой/битый пиксмап — предупреждение и выход (реф. пишет в лог
    // «Thumbnail Image is invalid.»); иначе масштаб с сохранением пропорций → в ячейку.
    if (index < 0 || index >= 4 || !m_cells[index])
        return;
    const QPixmap pm(imgPath);
    if (pm.isNull())
        return;
    m_cells[index]->setPixmap(pm.scaled(icon_, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_cells[index]->setAlignment(Qt::AlignCenter);
}

void KImgList::ClearImgInfo()
{
    // Реф. @0x67fef8: сброс всех четырёх превью.
    for (KImgListCell *c : m_cells)
        if (c)
            c->clear();
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
