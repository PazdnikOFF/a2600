#include "report/KRTDataSourceDemo.h"
#include "report/KReportDataSource.h"
#include "sys/KSystem.h"

#include <QDir>

KRTDataSourceDemo::KRTDataSourceDemo(const QString &reportRoot)
    : reportRoot_(reportRoot)
{
    if (reportRoot_.isEmpty())
        reportRoot_ = QDir(KSystem::SystemPath())
            .absoluteFilePath("presetdata/syspreset/mainapp/patient/report");
}

void KRTDataSourceDemo::Build(KReportDataSource &out, int imageCount) const
{
    // Образцовые данные пациента (реф. демо-набор редактора шаблонов).
    out.SetPatient("RT_PATIENT_NAME", QStringLiteral("Demo Patient"));
    out.SetPatient("RT_PATIENT_ID", QStringLiteral("00000000"));
    out.SetPatient("RT_PATIENT_GENDER", QStringLiteral("M"));
    out.SetPatient("RT_PATIENT_AGE", QStringLiteral("40"));
    out.SetPatient("RT_EXAMDATE", QStringLiteral("2026-01-01"));
    out.SetPatient("RT_DISEASE_NAME", QStringLiteral("Sample diagnosis"));
    out.SetPatient("RT_SURGERY_FINDING", QStringLiteral("Sample findings text"));
    out.SetPatient("RT_DIAGNOSIS", QStringLiteral("Sample conclusion"));
    out.SetPatient("RT_SUGGESTION", QStringLiteral("Sample suggestion"));

    // Демо-снимки из report/DemoImage/ImageN.png (циклически, если count > доступно).
    const QString demoDir = QDir(reportRoot_).absoluteFilePath("DemoImage");
    for (int i = 0; i < imageCount; ++i) {
        const QString path = QDir(demoDir).absoluteFilePath(QString("Image%1.png").arg(i % 10));
        out.SetImage(i, path, QString("Demo %1").arg(i + 1));
    }
}
