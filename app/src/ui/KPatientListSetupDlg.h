#pragma once

#include "KDialog.h"

class QCheckBox;

// Диалог настройки колонок списка пациентов / worklist (реф. KPatientListSetupDlg @ctor
// 0x7dc730, base KDialog, SetKStyle(7)=W1024, setupUi @0x7de198, title TR_PLSettings).
// UI-порт. НЕ 57, а 14 чекбоксов в двух группах (реф. абсолютная геометрия → в порте
// QGridLayout): grb_patientlist (9 колонок отображения, 2 из них — польз-поля с paired
// QLineEdit) + grb_worklist (5 колонок запроса, каждый с paired default-инпутом). Кнопки
// Save(TR_Sve)/Exit(TR_Ext); выход спрашивает «сохранить?». DEVICE-STUB: конфиг-хендлеры
// KPatientListConfigSetupHandler/KWorkListConfigSetupHandler + IPC PublishMsg (12004/07/08).
class KPatientListSetupDlg : public KDialog
{
    Q_OBJECT
public:
    explicit KPatientListSetupDlg(QWidget *parent = nullptr);

private slots:
    void SaveSetupData();   // реф. @0x7dc8e8: записать конфиг + broadcast (DEVICE-STUB)
    void ExitSetupData();   // реф. @0x7dd5a8: спросить сохранить? → save/close

private:
    void buildUi();
};
