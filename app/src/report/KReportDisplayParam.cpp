#include "report/KReportDisplayParam.h"
#include "report/KReportTemplate.h"
#include "report/KReportDataSource.h"

void KReportDisplayParam::Reset()
{
    valid_.clear();
}

bool KReportDisplayParam::AppendValidItem(const QString &name)
{
    if (name.isEmpty())
        return false;
    // реф.: если задано эталонное множество, элемент должен в него входить.
    if (!ref_.isEmpty() && !ref_.contains(name))
        return false;
    valid_.insert(name);
    return true;
}

bool KReportDisplayParam::IsItemValid(const QString &name) const
{
    return valid_.contains(name);
}

bool KReportDisplayParam::markItem(const ReportItem &it, const KReportDataSource &ds)
{
    // Сам элемент валиден, если у него есть непустое связанное значение.
    bool self = false;
    if (!it.dataSrc.isEmpty()) {
        const QString v = ds.GetValue(it.dataSource(), it.dataField());
        self = !v.isEmpty();
    }
    // Потомки: валиден, если валиден хотя бы один.
    bool anyChild = false;
    for (const ReportItem &c : it.children)
        if (markItem(c, ds))
            anyChild = true;

    const bool valid = self || anyChild;
    if (valid)
        AppendValidItem(it.name);
    return valid;
}

int KReportDisplayParam::UpdateTemplateDisplayParam(const QVector<ReportItem> &items,
                                                    const KReportDataSource &ds)
{
    Reset();
    for (const ReportItem &it : items)
        markItem(it, ds);
    return valid_.size();
}
