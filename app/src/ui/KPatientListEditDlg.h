#pragma once

#include "KDialog.h"

class QLineEdit;
class QComboBox;
class QPushButton;
class QDateEdit;
class KMemComboBox;
class KPatientDateEdit;

// Форма добавления/редактирования пациента (реф. KPatientListEditDlg @ctor 0x7cf010, base
// KDialog, setupUi @0x7d03a8, SetKStyle(7)=W1024, 1024×712, title TR_EPatient). UI-порт
// full-fidelity — реальные KMemComboBox (PID/имя/направитель) + KPatientDateEdit (ДР/план).
// Две группы: grp_patientinfo (3×3: PID/имя/пол/ДР/возраст/тел/койка/2 польз-поля) +
// grp_examinfo (2×2: обследование/план-дата/направитель/дата-заявки). Кнопки Exam/OK/Cancel.
// КЛЮЧЕВАЯ ЛОГИКА: двунаправленный пересчёт ДР↔Возраст (смена ДР → GetAge в поле возраста;
// правка возраста → ДР сбрасывается в InvalidDate). Валидаторы возраст [1-199]/тел [0-9\-].
// DEVICE-STUB: сохранение в БД (KPatientListDBTableHandler), MRU-персист, IPC PublishMsg,
// exam-workflow — заглушки.
class KPatientListEditDlg : public KDialog
{
    Q_OBJECT
public:
    explicit KPatientListEditDlg(const QString &patientKey = QString(), QWidget *parent = nullptr);

private slots:
    void OnChangedPatientDate(const QDate &dob);   // реф. @0x7ca218: ДР → возраст
    void OnChangedPatientAge();                     // реф. @0x7ca138: возраст → сброс ДР
    void OnBtnConfirm();
    void OnBtnCancel();

private:
    void buildUi();
    int GetAge(const QDate &dob) const;             // реф. @0x7ca090

    QLineEdit *m_editAge = nullptr;
    QLineEdit *m_editPhone = nullptr;
    QLineEdit *m_editBedno = nullptr;
    KMemComboBox *m_cmbPatientId = nullptr;
    KMemComboBox *m_cmbName = nullptr;
    KMemComboBox *m_cmbApplicant = nullptr;
    QComboBox *m_cmbGender = nullptr;
    QComboBox *m_cmbExamItem = nullptr;
    KPatientDateEdit *m_dateDob = nullptr;
    KPatientDateEdit *m_datePlantime = nullptr;
    QDateEdit *m_dateApplytime = nullptr;
    QString m_patientKey;
};
