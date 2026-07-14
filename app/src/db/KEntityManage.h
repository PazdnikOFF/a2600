#pragma once

#include <QString>
#include <QList>

// Сущности и доступ к БД пациентов/осмотров (реф. KEntityManage + *DBTableHandler).
// Бэкенд: SQLCipher на устройстве (libsqlcipher), в отладке — SQLite через Qt5::Sql.
// Поля таблиц — из реверса (PatientName/PatientSex/PatientAge/patientid/birthday,
// ExamDate/ExamTime/AccessionNumber). Запросы — как в оригинале
// (insert into %s (%s) values(%s); select %s from %s where (%s)).

struct PatientEntity {
    QString patientId;     // patientid
    QString patientName;   // PatientName
    QString patientSex;    // PatientSex
    QString patientAge;    // PatientAge
    QString birthday;      // birthday
};

struct ExamEntity {
    QString accessionNumber;  // AccessionNumber
    QString patientId;        // patientid (FK)
    QString examDate;         // ExamDate
    QString examTime;         // ExamTime
    QString examFolder;       // папка снимков осмотра
};

class KEntityManage
{
public:
    static KEntityManage &Instance();

    bool OpenDb(const QString &dbPath);   // открыть/создать БД (+ таблицы)
    void CloseDb();

    // Пациенты (реф. KPatientListDBTableHandler)
    bool AddPatientEntity(const PatientEntity &p);
    bool GetPatientEntity(const QString &patientId, PatientEntity &out) const;
    QList<PatientEntity> GetAllPatients() const;

    // Осмотры (реф. KExamListDBTableHandler)
    bool AddExamEntity(const ExamEntity &e);
    QList<ExamEntity> GetExamList(const QString &patientId) const;

private:
    KEntityManage() = default;
    bool createTables();
    bool opened_ = false;
};
