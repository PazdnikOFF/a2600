#pragma once

#include "ui/KDialog.h"

class QLabel;
class QCheckBox;
class QPushButton;
class QProgressBar;
class QFrame;

// Диалог прошивки/обновления разделов (реф. KUpdateAction : KDialog, ctor @0x6e6170). UI-порт.
// Ui_KUpdateAction::setupUi @0x6e71e0 (отдельный объект). Диалог 800×839, SetKStyle(FULLSCREEN),
// титул TR_Ugde. Список разделов (чекбокс + версия): app/hmi/pap/papp00-07(без 05)/papp80/lcd;
// снизу — кнопки Start/PowerOff, прогресс, сообщения.
//
// DEVICE-логика в порт НЕ тянется (только разметка): KUpdateConf (реальная запись раздела через
// KHalClass::execUpdateAction), GetUpdateVersion (версии в label_*), KProjectSet::IsShowPAPP
// (видимость PAPP-разделов), GetSystemStatus (видимость LCD), слоты StartUpdate/OneItem* —
// заглушки. Версии/чек-состояния/видимость наполняются динамически на устройстве.
class KUpdateAction : public KDialog
{
    Q_OBJECT
public:
    explicit KUpdateAction(QWidget *parent = nullptr);

private slots:
    void StartUpdate();        // реф.: запуск прошивки выбранных разделов (device)
    void ClickBtnPoweroff();   // реф.: выключение (device)
    void UpdateProgressUpd();  // реф.: тик прогресса по таймеру

private:
    void setupUi();
    void initWidget();

    QFrame       *frame_item = nullptr;
    QFrame       *frame_btn = nullptr;
    QLabel       *label_msg = nullptr;
    QLabel       *label_updmsg = nullptr;
    QProgressBar *progress_upd = nullptr;
    QPushButton  *btn_start = nullptr;
    QPushButton  *btn_poweroff = nullptr;
};
