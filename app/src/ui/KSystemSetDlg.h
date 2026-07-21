#pragma once

#include "ui/KDialog.h"

// Диалог системных настроек (реф. KSystemSetDlg : KDialog, ctor @0x5f5ed0,
// Ui_KSystemSetDlg::setupUi @0x5f7760). UI-порт. Диалог 566×1080, SetKStyle(W460),
// титул TR_SSettings. Это НЕ отдельные диалоги даты/времени/сети — всё это секции
// одного диалога в общем QGridLayout:
//   • Видео/UI  (TR_Vdeo2):  разрешение, форма угла, размер среза, режим объектива,
//                            сегментация видео — 5 комбобоксов;
//   • Язык/Дата (TR_LATime): язык, формат даты (комбо) + дата (QDateEdit) + время
//                            (QTimeEdit);
//   • Сеть      (TR_Ntwrk):  IP/маска/шлюз (KIpLineEdit→QLineEdit c inputMask) + MAC.
// Внизу: label_message (TR_TITEARestart — «требуется перезапуск») + панель кнопок
// (реф. KParamSetBtn: Общие/Параметры/Устройство + Default/Save/Exit).
//
// Кастом-виджеты заменены портируемыми: KLineH → QFrame(HLine); KIpLineEdit → QLineEdit
// c маской IP; KParamSetBtn → обычная панель кнопок.
//
// DEVICE в порт не тянется: KSystemSet (ini read/write), KNetWorkSet (NIC-конфиг),
// SetSystemTime (RTC), GetSaveVideoSplitList, GetSystemStatus/GetEndoScope/GetKAccount,
// SaveConfig/LoadDefaultConfig, тик системных часов — заглушки; поля пусты, Exit→close.
class KSystemSetDlg : public KDialog
{
    Q_OBJECT
public:
    explicit KSystemSetDlg(QWidget *parent = nullptr);

private:
    void setupUi();
};
