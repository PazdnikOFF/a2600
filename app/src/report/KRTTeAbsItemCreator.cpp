#include "report/KRTTeAbsItemCreator.h"

#include "report/KRTTeCreatorContext.h"
#include "report/KReportDisplayParam.h"
#include "report/KReportTemplateCommonDef.h"

#include <QCoreApplication>

// Реф. литерал ключа разрыва страницы (@0x866000; рядом лежит PageBreak_After @0x866018).
static const char *kPageBreakBefore = "PageBreak_Before";

void KRTTeAbsItemCreator::UpdateItemConfigPointer(const KReportTemplateItem &item)
{
    // Реф. @0x513278.
    m_curItemConfig = nullptr;
    KReportDisplayParam *dp = m_context.DisplayParam();
    if (!dp)
        return;
    const auto &cfgs = dp->ItemConfigs();
    auto it = cfgs.find(item.m_strID);
    if (it != cfgs.end())
        m_curItemConfig = &it->second;
}

std::string KRTTeAbsItemCreator::GetItemTitle(const KReportTemplateItem &item,
                                              const std::map<std::string, std::string> &unused) const
{
    // Реф. @0x5133a0: QObject::staticMetaObject.tr(item.m_strTitle) → UTF-8.
    // Карта-аргумент в реф. НЕ используется — сохраняем в сигнатуре ради верности.
    (void)unused;
    const QString tr = QCoreApplication::translate("QObject", item.m_strTitle.c_str());
    return tr.toUtf8().constData();
}

bool KRTTeAbsItemCreator::CheckCreate(const KReportTemplateItem &item,
                                      const std::map<std::string, std::string> &cfgMap)
{
    // Реф. @0x513530.
    KReportDisplayParam *dp = m_context.DisplayParam();
    if (!dp)
        return false;

    if (cfgMap.empty()) {
        // Ветка 1: валидность проверяется по ИМЕНИ элемента.
        if (!dp->IsItemValid(item.m_strID))
            return false;                       // реф.: выход БЕЗ UpdateItemConfigPointer
        UpdateItemConfigPointer(item);
        return true;
    }

    auto pb = cfgMap.find(kPageBreakBefore);
    if (pb == cfgMap.end()) {
        // Ветка 2: ключа разрыва нет → валидность ВООБЩЕ НЕ ПРОВЕРЯЕТСЯ.
        UpdateItemConfigPointer(item);
        return true;
    }

    // Ветка 3: ключ есть → валидность проверяется по source-id, собранному из значения.
    std::string sid;
    if (!report_template::ConvertToSourceID(pb->second, cfgMap, sid))
        sid = pb->second;
    if (!dp->IsItemValid(sid))
        return false;
    UpdateItemConfigPointer(item);
    return true;
}
