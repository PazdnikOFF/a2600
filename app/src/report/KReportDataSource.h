#pragma once

#include <QString>
#include <QHash>

// Источник данных отчёта (реф. KReportEditDataSource, X-2600). Значения полей
// адресуются парой "<источник>,<поле>" из DataSrc элемента шаблона:
//   RT_DATASOURCE_PATIENT   — данные пациента/осмотра (RT_PATIENT_NAME, RT_DIAGNOSIS…)
//   RT_DATASOURCE_PERIPHERAL — снимки и подписи (RT_TEST_IMAGE0.., RT_TEXT_MARK0..)
//   RT_DATASOURCE_REPORT/MEASURE — текст отчёта/измерения.
class KReportDataSource
{
public:
    void SetValue(const QString &source, const QString &field, const QString &value);
    QString GetValue(const QString &source, const QString &field) const;
    bool HasValue(const QString &source, const QString &field) const;

    // Удобные сеттеры под известные источники.
    void SetPatient(const QString &field, const QString &value)
    { SetValue("RT_DATASOURCE_PATIENT", field, value); }
    // Снимок N: путь к изображению + подпись (реф. RT_TEST_IMAGE<N>/RT_TEXT_MARK<N>).
    void SetImage(int idx, const QString &path, const QString &mark = QString());

private:
    QHash<QString, QString> values_;   // ключ "source|field"
    static QString key(const QString &s, const QString &f) { return s + "|" + f; }
};
