#pragma once

#include <QWidget>

// Вид списка обследований (реф. KExamListViewUi : QWidget + KObject-mixin — НЕ диалог!,
// ctor @0x801468, Ui_KExamListViewUi::setupUi @0x805258). UI-порт. Встраиваемый QWidget
// (реф. 1630×1034); порт как самостоятельный QWidget (базовый класс совпал). Структурный
// близнец KDicomQueueViewUi. grp_view → vbox → [widget_search (KExamListSearch→плейсхолдер),
// tableView (KTableView→QTableWidget, 20 колонок: чекбокс/PID/имя/пол/возраст/дата/
// направитель/врач/эндоскоп/SN/статус/ENo/картинки/видео/ДР/тел/койка/№рег/2польз),
// пейджер-бар (label записей/hint + home/pre/поле/номер/next/tail)]. Строки — device (БД).
//
// Кастом заменены: KTableView→QTableWidget, KPagePushButton→QPushButton (глиф), KPageLineEdit
// →QLineEdit, KExamListSearch→плейсхолдер, KObject-mixin опущен.
//
// DEVICE в порт не тянется: OnGetExamListDataFromDB/OnQuery (БД), SubscribeMsg×8,
// disk-capacity метки, custom-field заголовки (KPatientListConfigSetupHandler), пагинация
// — заглушки. Строки пусты.
class KExamListViewUi : public QWidget
{
    Q_OBJECT
public:
    explicit KExamListViewUi(QWidget *parent = nullptr);

private:
    void setupUi();
};
