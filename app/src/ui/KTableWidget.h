#pragma once

#include <QTableWidget>

// Таблица прошивки (реф. KTableWidget : QTableWidget, ctor @0x6876a0, sizeof 0x30 = размер
// базы). UI-порт. Класс НАМЕРЕННО пустой: ctor ничего не настраивает (все настройки делает
// владелец, см. Ui_KImgList::setupUi @0x681018), своих полей и метаданных нет.
//
// Единственный смысл класса — ОДНА строка в keyPressEvent @0x6876d8: `b QAbstractItemView::
// keyPressEvent`, то есть квалифицированный вызов ДЕДА в обход QTableView/QTableWidget.
// Практический эффект: пропадает табличная навигация QTableView по стрелкам/Tab (её делает
// QTableView::keyPressEvent), остаётся только базовая обработка QAbstractItemView.
class KTableWidget : public QTableWidget
{
    Q_OBJECT
public:
    explicit KTableWidget(QWidget *parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *e) override;   // реф. @0x6876d8 — ДЕД, не прямая база
};
