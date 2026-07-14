#pragma once

#include <QString>
#include <QList>

// Полная сущность/CRUD осмотра (реф. KEntityExam + KExamListDBTableHandler,
// tb_ExamList, X-2600). Расширяет базовый tb_Exam из KEntityManage: полный набор
// колонок + пагинация (GetPageRecordFromDb) + последний ExamId. Ключ — ExamId.
// Колонки из реверса: ExamId, AccessionNumber, PatientId, ExamType, ExamDate,
// ExamTime, ExamStatus, RegisterNumber, DrExamId, ExamDir.
struct ExamListEntity {
    QString examId;            // m_strExamId (ключ)
    QString accessionNumber;   // AccessionNumber
    QString patientId;         // PatientId
    QString examType;          // ExamType
    QString examDate;          // ExamDate
    QString examTime;          // ExamTime
    int     examStatus = 0;    // ExamStatus (0=новый,1=выполнен,2=завершён…)
    QString registerNumber;    // RegisterNumber
    QString drExamId;          // DrExamId (врач осмотра)
    QString examDir;           // ExamDir (папка снимков)
};

class KEntityExam
{
public:
    // Работает на соединении KEntityManage (одна БД).
    explicit KEntityExam(const QString &connectionName = "endo_main");

    bool CreateTable() const;
    bool CreateEntity(const ExamListEntity &e);          // реф. CreateEntity/AddExamEntity
    bool UpdateEntity(const ExamListEntity &e);          // реф. UpdateEntity
    bool DeleteSelf(const QString &examId);              // реф. DeleteSelf/DeleteSingleEntity
    bool GetEntityDetail(const QString &examId, ExamListEntity &out) const; // реф. GetEntityDetail
    QList<ExamListEntity> GetEntityDetailList(const QString &patientId) const; // осмотры пациента
    QList<ExamListEntity> GetEntitySummaryList() const;  // реф. GetEntitySummaryList
    int  GetEntityNumber() const;                        // реф. GetEntityNumber
    // Последний ExamId (реф. GetLatestExamIdFromDb).
    QString GetLatestExamId() const;
    // Постраничная выборка (реф. GetPageRecordFromDb): offset/limit.
    QList<ExamListEntity> GetPageRecord(int offset, int limit) const;

private:
    ExamListEntity fromQuery(const class QSqlQuery &q) const;
    QString conn_;
};
