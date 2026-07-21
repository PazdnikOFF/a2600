#pragma once

#include "ui/KDialog.h"

class QLineEdit;
class QDateEdit;

// Диалог ввода нового пациента (реф. KPatientListAddDlg : KDialog, ctor @0x7c2a60,
// Ui_KPatientListAddDlg::setupUi @0x7c41b0). UI-порт. Немодальный, фикс. 1024×712,
// SetKStyle(W1024), титул TR_APatient. Абсолютная геометрия (без layout'ов): две
// группы — «инфо пациента» (ID/имя/пол/ДР/возраст/тел/койка/2 польз-поля, сетка 3×3)
// и «инфо обследования» (пункт/план-дата/направитель/дата-заявки); снизу подсказка
// TR_RField + кнопки Exam/OK/Cancel. Возраст↔ДР пересчитываются взаимно (чистый UI).
//
// Кастом заменены: KMemComboBox→editable QComboBox+maxLength; KPatientDateEdit→QDateEdit.
//
// DEVICE в порт не тянется: OnBtnConfirm/OnBtnExam (persist в БД + запуск обследования),
// MRU-списки cmb_patientid/name/applicant + OnChangedPatientId/Name (поиск в БД),
// cmb_examitem (GetEndoClassToQstring), заголовки/видимость польз-полей и групп
// (KPatientListConfigSetupHandler), формат/границы дат (KSystemSet) — заглушки.
// Cancel→close.
class KPatientListAddDlg : public KDialog
{
    Q_OBJECT
public:
    explicit KPatientListAddDlg(QWidget *parent = nullptr);

private:
    void setupUi();

    QLineEdit *m_editAge = nullptr;
    QDateEdit *m_dateDob = nullptr;
};
