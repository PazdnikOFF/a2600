#pragma once

#include <QString>

class KReportDataSource;

// Реальный источник данных отчёта (реф. KRTDataSourceReal : KRTAbsDataSource, X-2600).
// Заполняет KReportDataSource данными из БД: пациент (KEntityManage), осмотр
// (KEntityExam), содержимое отчёта (KEntityReport) → поля RT_DATASOURCE_PATIENT.
// В отличие от demo-источника, тянет живые записи. Далее KReportHtmlGenerator
// строит документ из этих данных.
class KRTDataSourceReal
{
public:
    explicit KRTDataSourceReal(const QString &connectionName = "endo_main");

    // Собрать источник данных для отчёта осмотра (по AccessionNumber + PatientId).
    // Заполняет RT_PATIENT_* из tb_Patient, RT_EXAMDATE из tb_ExamList/tb_Exam,
    // RT_DIAGNOSIS/RT_SURGERY_FINDING/RT_DISEASE_NAME/RT_SUGGESTION из tb_Report.
    bool Build(const QString &patientId, const QString &accessionNumber,
               KReportDataSource &out) const;

private:
    QString conn_;
};
