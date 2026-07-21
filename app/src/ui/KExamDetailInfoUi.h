#pragma once

#include "ui/KDialog.h"

// Панель деталей обследования (реф. KExamDetailInfoUi : KFullScreenDialog — НЕ KDialog!,
// ctor @0x495c98, Ui_KExamDetailInfoUi::setupUi @0x49b498). UI-порт. Полноэкранная
// 1280×960 (реф. base KFullScreenDialog(parent,2004), 3-й арг = int-id 2004, не bool) →
// портируем над KDialog(FULLSCREEN). SetKStyle НЕТ, титул SetTitle(TR_Dtls).
// Слева widget_left_opts (фикс 260): 4 больших action-кнопки 212² (Report/Export/Upload/
// Delete) + label_disk_vol (250², объём диска device) + Exit. Справа: шапка пациента
// (имя/дата) + widget_tableView (таблица файлов, device) + play-bar пагинации
// (Total/hint + head/pre/edit_page/«/1»/next/tail).
//
// Кастом заменены: KFullScreenDialog→KDialog(FULLSCREEN); KImgPushButton(head/pre/next/
// tail)→QPushButton c глифами; таблица→QTableView-плейсхолдер.
//
// DEVICE в порт не тянется: слоты кнопок (OpenReportEdit/Export/Upload/OpenDeleteDlg),
// модель таблицы, disk-vol калькулятор, SubscribeMsg(0x2f07/0x2f0b), пагинация — заглушки.
// Exit→close.
class KExamDetailInfoUi : public KDialog
{
    Q_OBJECT
public:
    explicit KExamDetailInfoUi(QWidget *parent = nullptr);

private:
    void setupUi();
};
