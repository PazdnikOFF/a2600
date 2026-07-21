#pragma once

#include "ui/KDialog.h"

// Диалог просмотра логов (реф. KLogView : KDialog, ctor @0x712f40, Ui_KLogView::setupUi
// @0x7138b8). UI-порт. Немодальный, 800×600, SetKStyle(W460), титул SetTitle(TR_LView).
// Постраничный просмотрщик лог-файлов: textBrowser (QTextBrowser, lineWidth 2) + ряд
// кнопок навигации «<<»/«>>» (страницы) + btn_fullscreen(TR_FScreen, роль==4) + «|<<»/
// «>>|» (переключение лог-файлов). Все stock Qt, кастомов и device-IPC НЕТ — бэкенд
// только локальный QFile (перечисление/чтение логов).
//
// DEVICE/файловая логика заглушена: UpdateFileNameList/OpenNewFile/ShowLogText (чтение
// логов с диска), Pre/NextPageView (пейджинг), Pre/NextLogView (смена файла),
// роль-гейтинг btn_fullscreen. Пейджинг-кнопки подключены на скролл (чистый UI).
class KLogView : public KDialog
{
    Q_OBJECT
public:
    explicit KLogView(QWidget *parent = nullptr);

private:
    void setupUi();
};
