#include "KTableWidget.h"

KTableWidget::KTableWidget(QWidget *parent)
    : QTableWidget(parent)
{
    // Реф. ctor @0x6876a0: только вызов базы. Ни одной настройки, ни одной константы.
}

void KTableWidget::keyPressEvent(QKeyEvent *e)
{
    // Реф. @0x6876d8 — ровно одна инструкция `b QAbstractItemView::keyPressEvent`.
    QAbstractItemView::keyPressEvent(e);
}
