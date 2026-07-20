#pragma once

#include <QList>
#include <QString>

// Сущность/CRUD пациента (реф. KEntityPatient + KPatientListDBTableHandler,
// tb_PatientList, X-2600). Чистый SQLite-слой (не UI/не device), соединение
// endo_main (файл HD-2000.dat).
//
// Колонки реверснуты из KPatientEntry::ConvertToMap (порядок сохранён). В реф.
// все поля std::string; у нас QString — для консистентности с DB-слоем проекта.
// PK — технический `id` (AUTOINCREMENT); PatientID — бизнес-ключ пациента.
// Типы колонок в реф. не заданы в этих классах (DDL живёт в KEntityManage) —
// приняты TEXT / id INTEGER (единственное «додумывание», помечено).
struct KPatientEntry {
    QString id;               // технический PK (пусто при вставке → AUTOINCREMENT)
    QString patientID;        // PatientID (бизнес-ключ)
    QString patientName;      // PatientName
    QString patientSex;       // PatientSex
    QString patientBirthday;  // PatientBirthday
    QString applicantDate;    // ApplicantDate
    QString applicants;       // Applicants
    QString planDate;         // PlanDate (реф. 0xc8; добавлено при реверсе KExamBussinessHandler)
    QString userItem1;        // UserItem1
    QString userItem2;        // UserItem2
    QString sickBedId;        // SickBedId
    QString telephoneNumber;  // TelephoneNumber
    QString registerNumber;   // RegisterNumber
    QString worklistUID;      // WorklistUID
    QString patientAge;       // PatientAge (в реф. int, сентинел -1)
    QString examStatus;       // ExamStatus (в реф. int, сентинел -1)
    QString examType;         // ExamType  (реф. 0x84, int; добавлено при реверсе
                              //            KExamBussinessHandler::FinishSaveDataAction)
};

// Сущность (реф. KEntityPatient : KEntityBase, таблица tb_PatientList, вид VPatientList).
class KEntityPatient
{
public:
    explicit KEntityPatient(const QString &connectionName = "endo_main");

    bool CreateTable() const;
    bool CreateEntity(const KPatientEntry &e);                 // реф. CreateEntity
    bool UpdateEntity(const QString &id, const KPatientEntry &e);   // реф. UpdateEntity
    bool DeleteSelf(const QString &id);                        // реф. DeleteSelf (PK=id)
    bool GetEntityDetail(const QString &id, KPatientEntry &out) const;
    QList<KPatientEntry> GetEntityDetailList() const;          // реф. GetEntityDetailList (VPatientList)
    int  GetEntityNumber() const;                             // реф. GetEntityNumber

private:
    KPatientEntry fromQuery(const class QSqlQuery &q) const;
    QString conn_;
};

// CRUD-хендлер (реф. KPatientListDBTableHandler) — тонкая обёртка над KEntityPatient
// (в реф. — над реестровым KEntityManage по type="Patient"; у нас connection-based).
class KPatientListDBTableHandler
{
public:
    explicit KPatientListDBTableHandler(const QString &connectionName = "endo_main");

    int  GetRecordNumber() const;
    // 0 — найдено (out заполнен); -1 — нет записи (реф.).
    int  GetEntity(const QString &id, KPatientEntry &out) const;
    bool AddNewPatientEntity(const KPatientEntry &e);
    bool UpdatePatientEntity(const QString &id, const KPatientEntry &e);
    bool DeleteEntity(const QString &id);
    // Реф. DeleteEntites(vector) — ЗАГЛУШКА (тело возвращает глоб. int, ничего не удаляет).
    // Воспроизведено как no-op с явной пометкой.
    bool DeleteEntites(const QList<QString> &ids);
    // ExamStatus по id (реф. GetEntityDetail → map["ExamStatus"]=<st> → UpdateEntity).
    bool UpdateExamStatus(const QString &id, int status);
    QList<KPatientEntry> GetPageRecordFromDb() const;   // реф. → GetEntitySummaryList

private:
    KEntityPatient ent_;
};
