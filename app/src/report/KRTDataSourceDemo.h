#pragma once

#include <QString>

class KReportDataSource;

// Демо-источник данных отчёта (реф. KRTDataSourceDemo : KRTAbsDataSource, X-2600).
// Заполняет KReportDataSource образцовыми данными пациента + демо-снимками из
// report/DemoImage/ (Image0..9.png, HospitalLogo.png) — для ПРЕВЬЮ шаблонов в
// редакторе (без реального осмотра). Пара к KRTDataSourceReal.
class KRTDataSourceDemo
{
public:
    // reportRoot — .../mainapp/patient/report (DemoImage внутри). По умолчанию — из KSystem.
    explicit KRTDataSourceDemo(const QString &reportRoot = QString());

    // Собрать демо-датасорс: образцовый пациент + N демо-снимков (для сетки шаблона).
    void Build(KReportDataSource &out, int imageCount = 4) const;

private:
    QString reportRoot_;
};
