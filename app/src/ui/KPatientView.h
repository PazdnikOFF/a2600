#pragma once

#include <QWidget>
#include <QString>

class QLineEdit;
class QComboBox;
class QVBoxLayout;
class KMemComboBox;
class KSpinAge;
class KEmpDateEdit;

// Демо-данные пациента (реф. MainUiPatientInfo, size 0x128 — inline в KPatientView@0xd0).
struct KPatientInfo {
    QString name;
    int age = 0;
    int genderCode = 3;   // 3=blank/1=M/0=F/2=Unknown
    QString dob;          // "yyyy-MM-dd"
    QString applicant;
    QString patientId;
    QString examNo;       // read-only (accession)
    QString doctor;       // read-only (внешний источник)
    QString custom1, custom2;
};

// Форма инфо о пациенте (реф. KPatientView : QWidget + KObject, ctor @..., size 0x1f8). UI-порт.
// НЕ QLayout в реф. (абсолютный стек, шаг 49px, фикс-ширина 178, иконки-оверлеи); в порте —
// QVBoxLayout из строк [иконка | редактор]. Поля (в порядке показа): Name(KMemComboBox)/
// Gender(QComboBox 4)/Age(KSpinAge)/DoB(KEmpDateEdit)/Applicant/PatientID(KMemComboBox)/
// ExamNo(ro)/Custom1/Custom2. Doctor — опц./скрыт. Иконки qss/black/icon/patient/*.png.
// DEVICE-seam: реф. LoadPatientInfo/SavePatientData через KExamBussinessHandler (DB) + KObject-
// шина → в порте setPatient(struct)/GetPatient() + сигнал patientSaved. KMemComboBox DB-автодоп —
// no-op. 100% PORT UI (данные — стаб).
class KPatientView : public QWidget
{
    Q_OBJECT
public:
    explicit KPatientView(QWidget *parent = nullptr);

    void setPatient(const KPatientInfo &info);   // реф. LoadPatientInfo (DB-seam → инъект)
    KPatientInfo GetPatient() const;             // реф. SavePatientData (чтение полей)
    void ClearWidgetInfo();                      // реф. @ClearWidgetInfo

signals:
    void patientSaved(const KPatientInfo &info);

private:
    QWidget *makeRow(const QString &iconName, QWidget *editor);   // реф. SetItemIcon + move

    KMemComboBox *m_name = nullptr;
    QComboBox *m_gender = nullptr;
    KSpinAge *m_age = nullptr;
    KEmpDateEdit *m_dob = nullptr;
    KMemComboBox *m_applicant = nullptr;
    KMemComboBox *m_patientId = nullptr;
    QLineEdit *m_examNo = nullptr;
    QLineEdit *m_doctor = nullptr;
    QLineEdit *m_custom1 = nullptr;
    QLineEdit *m_custom2 = nullptr;
};
