#pragma once

#include <QString>
#include <QStringList>
#include <QList>

// Сущность и БД-слой отчётов (реф. KEntityReport/KReportDBTableHandler, tb_Report).
// Хранит редактируемый текст отчёта осмотра (поля из ExamInfo.xml, источник
// RT_DATASOURCE_PATIENT) + имя шаблона. Ключ — AccessionNumber осмотра.
struct ReportEntity {
    QString accessionNumber;   // ключ (осмотр)
    QString templateName;      // выбранный шаблон (NP-2x2…)
    QString examView;          // RT_EXAM_VIEW
    QString diagnosis;         // RT_DIAGNOSIS (заключение)
    QString diseaseName;       // RT_DISEASE_NAME
    QString surgicalMethod;    // RT_SURGICAL_METHOD
    QString surgeryFinding;    // RT_SURGERY_FINDING
    QString suggestion;        // RT_SUGGESTION
    QString biopsy;            // RT_BIOPSY
    QString hp;                // RT_HP
};

class KEntityReport
{
public:
    static KEntityReport &Instance();

    bool OpenDb(const QString &dbPath);
    void CloseDb();

    // реф. CreateEntity/UpdateEntity/GetEntityDetail/DeleteSelf.
    bool SaveReport(const ReportEntity &r);           // insert or replace
    bool GetReport(const QString &accessionNumber, ReportEntity &out) const;
    bool DeleteReport(const QString &accessionNumber);
    int  GetReportNumber() const;

    // Постраничный доступ / запросы (реф. KReportDBTableHandler).
    // Страница отчётов (реф. GetPageRecordFromDb): offset/limit, порядок по ключу.
    QList<ReportEntity> GetPageRecord(int offset, int limit) const;
    // Все ключи отчётов (реф. GetAllRecordMainKey).
    QStringList GetAllRecordMainKey() const;
    // Число отчётов, где diagnosis/diseaseName содержат keyword (реф. GetQueryRecordNum).
    // Пустой keyword → все.
    int GetQueryRecordNum(const QString &keyword) const;
    // Постраничная выборка с фильтром по keyword (реф. GetPageRecordFromDb с запросом).
    QList<ReportEntity> QueryPageRecord(const QString &keyword, int offset, int limit) const;

private:
    KEntityReport() = default;
    bool createTables();
};
