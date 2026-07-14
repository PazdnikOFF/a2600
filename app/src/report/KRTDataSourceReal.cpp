#include "report/KRTDataSourceReal.h"
#include "report/KReportDataSource.h"
#include "db/KEntityManage.h"
#include "db/KEntityExam.h"
#include "report/KEntityReport.h"

KRTDataSourceReal::KRTDataSourceReal(const QString &connectionName)
    : conn_(connectionName)
{
}

bool KRTDataSourceReal::Build(const QString &patientId, const QString &accessionNumber,
                              KReportDataSource &out) const
{
    // Пациент (реф. RT_DATASOURCE_PATIENT ← tb_Patient).
    PatientEntity p;
    const bool pOk = KEntityManage::Instance().GetPatientEntity(patientId, p);
    if (pOk) {
        out.SetPatient("RT_PATIENT_NAME", p.patientName);
        out.SetPatient("RT_PATIENT_ID", p.patientId);
        out.SetPatient("RT_PATIENT_GENDER", p.patientSex);
        out.SetPatient("RT_PATIENT_AGE", p.patientAge);
        out.SetPatient("RT_BIRTHDAY", p.birthday);
    }

    // Осмотр (реф. RT_EXAMDATE ← tb_ExamList).
    KEntityExam ex(conn_);
    ExamListEntity e;
    if (ex.GetEntityDetail(accessionNumber, e) || !e.examDate.isEmpty())
        out.SetPatient("RT_EXAMDATE", e.examDate);

    // Содержимое отчёта (реф. RT_DIAGNOSIS/… ← tb_Report).
    ReportEntity r;
    const bool rOk = KEntityReport::Instance().GetReport(accessionNumber, r);
    if (rOk) {
        out.SetPatient("RT_DISEASE_NAME", r.diseaseName);
        out.SetPatient("RT_SURGERY_FINDING", r.surgeryFinding);
        out.SetPatient("RT_DIAGNOSIS", r.diagnosis);
        out.SetPatient("RT_SUGGESTION", r.suggestion);
        out.SetPatient("RT_EXAM_VIEW", r.examView);
    }

    return pOk || rOk;
}
