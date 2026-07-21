#pragma once

#include "ui/KDialog.h"

class QFrame;
class QToolButton;
class QLabel;
class QProgressBar;
class QTimer;

// Диалог подготовки/распаковки пакета обновления (реф. KUpdatePrepare : KDialog, X-2600).
// ПЕРВЫЙ UI-порт. Раскладка виджетов восстановлена из Ui_KUpdatePrepare::setupUi @0x6e2410
// и ctor @0x6e2130. Реф. базовый класс — КАСТОМНЫЙ KDialog (рамка/титул/SetKStyle) ещё НЕ
// портирован; здесь подставлен QDialog — клиентская область (сетка виджетов) совпадает 1:1,
// хром окна добавится с портом KDialog.
//
// Дерево (QGridLayout, разрежённые строки для вертикального разнесения):
//   (2,1) frame_update [StyledPanel/Raised, min 300×50] → QHBoxLayout → btn_update
//         [QToolButton, фикс 150×40, текст TR_Ugde]; по бокам (2,0)/(2,2) — h-спейсеры центрируют
//   (6,1) label_msg [QLabel, пусто, AlignHCenter|AlignTop] — заполняется в рантайме
//   (7,1) progress_rar [QProgressBar, max-width 600, формат "%p%"] — initWidget прячет, value 0, max 40
//   (1,1)/(9,1) v-спейсеры. Титул окна = TR_Ugde (реф. ctor перекрывает TR_Dlg из setupUi).
// setStyleSheet НЕ вызывается нигде — весь вид от KDialog::SetKStyle(1).
class KUpdatePrepare : public KDialog
{
    Q_OBJECT
public:
    explicit KUpdatePrepare(QWidget *parent = nullptr);

private slots:
    // Реф. @0x6e1f70: путь пакета с USB → StartDecompress; ошибка → label_msg + reconnect.
    void StartUpdate();
    // Реф. @0x6e1b08: ExecRarCmd (unrar) + запуск таймера; distro на USB/fs — device.
    void StartDecompress();
    // Реф. @0x6e15a0: инкремент прогресса распаковки по таймеру.
    void UpdateProgressDec();

private:
    void setupUi();      // реф. Ui_KUpdatePrepare::setupUi
    void initWidget();   // реф. KUpdatePrepare::initWidget @0x6e0d78

    QFrame       *frame_update = nullptr;
    QToolButton  *btn_update = nullptr;
    QLabel       *label_msg = nullptr;
    QProgressBar *progress_rar = nullptr;
    QTimer       *timer = nullptr;
    int           m_state = 0;   // реф. this+0x58
};
