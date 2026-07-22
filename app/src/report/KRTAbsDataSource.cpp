#include "KRTAbsDataSource.h"
#include "KReportTemplateCommonDef.h"

bool KRTAbsDataSource::SplitDataSrcID(const std::string &id, std::string &outA, std::string &outB) const
{
    // Реф. @0x589768: RevertPathByID(id, ",") → части; >=2 → outA=part0, outB=part1, true.
    const std::vector<std::string> parts = report_template::RevertPathByID(id, std::string(","));
    if (parts.size() < 2)
        return false;
    outA = parts[0];
    outB = parts[1];
    return true;
}
