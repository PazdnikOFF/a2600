#include "KLineH.h"

// Реф. ctor'ы делают ровно три вызова после базового QFrame(parent,0):
// setStyleSheet(<градиент>) + min/max по фикс-оси = 2px. Литералы градиентов —
// @0x88b4c0 (H) и парный (V) — перенесены дословно.

KLineH::KLineH(QWidget *parent)
    : QFrame(parent)
{
    setStyleSheet(QStringLiteral(
        "background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,"
        " stop:0 rgba(1, 1, 1, 255), stop:0.48 rgba(1, 1, 1, 255),"
        " stop:0.52 rgba(59, 59, 59, 255), stop:1 rgba(59, 59, 59, 255));"));
    setMaximumHeight(2);
    setMinimumHeight(2);
}

KLineV::KLineV(QWidget *parent)
    : QFrame(parent)
{
    setStyleSheet(QStringLiteral(
        "background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0,"
        " stop:0 rgba(59, 59, 59, 255), stop:0.48 rgba(59, 59, 59, 255),"
        " stop:0.52 rgba(1, 1, 1, 255), stop:1 rgba(1, 1, 1, 255));"));
    setMaximumWidth(2);
    setMinimumWidth(2);
}
