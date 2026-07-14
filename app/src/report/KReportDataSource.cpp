#include "report/KReportDataSource.h"

void KReportDataSource::SetValue(const QString &source, const QString &field,
                                 const QString &value)
{
    values_.insert(key(source, field), value);
}

QString KReportDataSource::GetValue(const QString &source, const QString &field) const
{
    return values_.value(key(source, field));
}

bool KReportDataSource::HasValue(const QString &source, const QString &field) const
{
    return values_.contains(key(source, field));
}

void KReportDataSource::SetImage(int idx, const QString &path, const QString &mark)
{
    SetValue("RT_DATASOURCE_PERIPHERAL", QString("RT_TEST_IMAGE%1").arg(idx), path);
    if (!mark.isEmpty())
        SetValue("RT_DATASOURCE_PERIPHERAL", QString("RT_TEXT_MARK%1").arg(idx), mark);
}
