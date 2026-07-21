#pragma once

#include <QWidget>

// Вид очереди DICOM-заданий (реф. KDicomQueueViewUi : QWidget + KObject-mixin — НЕ диалог!,
// ctor @0x8102f8, Ui_KDicomQueueViewUi::setupUi @0x812728). UI-порт. Встраиваемый QWidget
// (реф. 1630×1034); порт как самостоятельный QWidget (базовый класс совпал).
// Структура: grp_view → vbox → [widget_search (KDicomQueueSearch — фильтр), tableView
// (KTableView, 11 колонок очереди), пейджер-бар (label записей + |</</>/>| + поле страницы
// + номер страницы + hint)]. Строки таблицы — device (БД очереди).
//
// Кастом заменены: KTableView→QTableWidget, KPagePushButton→QPushButton (icon-only, у нас
// глиф-замена), KPageLineEdit→QLineEdit, KDicomQueueSearch→плейсхолдер-фильтр.
//
// DEVICE в порт не тянется: OnGetDicomQueueDataFromDB/OnQuery (БД), OnResendSelectedData
// (пере-отправка DICOM), SubscribeMsg/HandleSubscribeMsg (шина) — заглушки. Пейджинг/
// выделение/сортировка — чистый UI. Сосед (не цель): KDicomQueueOptUi (панель кнопок).
class KDicomQueueViewUi : public QWidget
{
    Q_OBJECT
public:
    explicit KDicomQueueViewUi(QWidget *parent = nullptr);

private:
    void setupUi();
};
