#include "report/KRTSimpleAbsItemCreator.h"

#include "report/KMeaStringUtil.h"
#include "report/KRTAbsDataSource.h"
#include "report/KRTSimpleCreatorContext.h"
#include "report/KReportDisplayParam.h"
#include "report/KReportTemplateCommonDef.h"

// ── Общая часть ──

std::string KRTSimpleAbsItemCreator::Resolve(const std::string &field,
                                             const std::map<std::string, std::string> &params)
{
    // Реф.: конверсия делается ТОЛЬКО при непустом params (проверка `ldr x0,[x20,#40]`).
    if (params.empty())
        return field;
    std::string out;
    if (report_template::ConvertToSourceID(field, params, out))
        return out;
    return field;
}

void KRTSimpleAbsItemCreator::AppendValid(const std::string &id) const
{
    if (KReportDisplayParam *dp = m_ctx.DisplayParam())
        dp->AppendValidItem(id);   // реф. @0x506ca0
}

namespace rt_simple_log {
namespace {
std::vector<CreatedRecord> &Storage()
{
    static std::vector<CreatedRecord> s;
    return s;
}
}   // namespace
const std::vector<CreatedRecord> &Records() { return Storage(); }
void Clear() { Storage().clear(); }
void Append(const std::string &creator, const std::string &itemId,
            const std::map<std::string, std::string> &params)
{
    Storage().push_back({creator, itemId, params});
}
}   // namespace rt_simple_log

// ── RT_TEXT_BLOCK (реф. @0x50b408) ──

int KRTSimpleTextItemCreator::CreateItem(const KReportTemplateItem &item,
                                         const std::map<std::string, std::string> &params)
{
    rt_simple_log::Append("KRTSimpleTextItemCreator", item.m_strID, params);
    const std::string id = Resolve(item.m_strID, params);

    // Реф.: проверяется ОРИГИНАЛЬНЫЙ item.m_strDataSrc (`ldr x1,[x21,#136]; cbz`).
    // Пустой → источник данных НЕ опрашивается вовсе, элемент СРАЗУ валиден.
    if (item.m_strDataSrc.empty()) {
        AppendValid(id);
        return 1;
    }
    std::string out;
    KRTAbsDataSource *ds = m_ctx.DataSource();
    if (ds && ds->GetTextData(Resolve(item.m_strDataSrc, params), out)) {   // слот 3
        AppendValid(id);
        return 1;
    }
    return 0;
}

// ── RT_TEXTGROUP_BLOCK (реф. @0x50b158) ──

int KRTSimpleTextGroupCreator::CreateItem(const KReportTemplateItem &item,
                                          const std::map<std::string, std::string> &params)
{
    rt_simple_log::Append("KRTSimpleTextGroupCreator", item.m_strID, params);
    // Реф.: ранний выход при пустом m_strDataSrc — 0 (в ОТЛИЧИЕ от текстового творца,
    // который в этом случае возвращает 1).
    if (item.m_strDataSrc.empty())
        return 0;

    std::vector<std::pair<std::string, std::string>> out;
    KRTAbsDataSource *ds = m_ctx.DataSource();
    if (ds && ds->GetTextGroupData(Resolve(item.m_strDataSrc, params), out)) {   // слот 5
        AppendValid(Resolve(item.m_strID, params));
        return 1;
    }
    return 0;
}

// ── RT_IMAGE_BLOCK (реф. @0x50a758) ──

namespace {
std::set<std::string> &LocalPicStorage()
{
    static std::set<std::string> s;
    return s;
}
}   // namespace

void KRTSimpleImageItemCreator::SetLocalPicNames(const std::set<std::string> &names)
{
    LocalPicStorage() = names;
}

const std::set<std::string> &KRTSimpleImageItemCreator::LocalPicNames()
{
    return LocalPicStorage();
}

int KRTSimpleImageItemCreator::CreateItem(const KReportTemplateItem &item,
                                          const std::map<std::string, std::string> &params)
{
    rt_simple_log::Append("KRTSimpleImageItemCreator", item.m_strID, params);
    const std::string id = Resolve(item.m_strID, params);

    // Реф.: последний токен id по разделителю '/' (иммедиата 0x2f @0x50a7d4) ищется
    // в статической карте локальных картинок MAP_LOCAL_PIC_PFINFO (@0xa78f98).
    // Найден → источник данных НЕ опрашивается, элемент сразу валиден.
    KMeaStringUtil util;
    const std::vector<std::string> toks = util.SplitStr(id, "/");
    if (!toks.empty() && LocalPicNames().count(toks.back())) {
        AppendValid(id);
        return 1;
    }

    std::string out;
    KRTAbsDataSource *ds = m_ctx.DataSource();
    if (ds && ds->GetImageData(Resolve(item.m_strDataSrc, params), out)) {   // слот 6
        AppendValid(id);
        return 1;
    }
    return 0;
}

// ── RT_IMAGEGROUP_BLOCK (реф. @0x50a4a8) ──

int KRTSimpleImageGroupCreator::CreateItem(const KReportTemplateItem &item,
                                           const std::map<std::string, std::string> &params)
{
    rt_simple_log::Append("KRTSimpleImageGroupCreator", item.m_strID, params);
    if (item.m_strDataSrc.empty())
        return 0;

    std::vector<std::pair<std::string, std::string>> out;
    KRTAbsDataSource *ds = m_ctx.DataSource();
    if (ds && ds->GetImageGroupData(Resolve(item.m_strDataSrc, params), out)) {   // слот 7
        AppendValid(Resolve(item.m_strID, params));
        return 1;
    }
    return 0;
}

// ── RT_TABLE_BLOCK / TITLE_TABLE / ROW_TABLE / OB_Z_SCORE (реф. @0x50af00) ──

int KRTSimpleTableItemCreator::CreateItem(const KReportTemplateItem &item,
                                          const std::map<std::string, std::string> &params)
{
    rt_simple_log::Append("KRTSimpleTableItemCreator", item.m_strID, params);
    // Реф.: к источнику данных НЕ обращается вообще; m_strDataSrc не читает.
    if (item.m_lstSubItems.empty())
        return 0;

    int total = 0;
    for (const KReportTemplateItem &sub : item.m_lstSubItems)
        total += m_ctx.CreateItem(sub, params);   // рекурсия через контекст

    if (total > 0)
        AppendValid(Resolve(item.m_strID, params));
    return total;   // ⚠️ возвращается СУММА, а не 0/1
}

// ── RT_SUB_DATA_BLOCK (реф. @0x50ac20) ──

int KRTSimpleSubDataItemCreator::CreateItem(const KReportTemplateItem &item,
                                            const std::map<std::string, std::string> &params)
{
    rt_simple_log::Append("KRTSimpleSubDataItemCreator", item.m_strID, params);
    if (item.m_strDataSrc.empty())
        return 0;

    KRTAbsDataSource *ds = m_ctx.DataSource();
    KReportTemplateDataNew sub;
    // Реф.: id берётся из m_strDataSrc, а НЕ из m_strID (слот 8).
    if (!ds || !ds->GetSubData(Resolve(item.m_strDataSrc, params), sub))
        return 0;

    // Реф.: конфиги вложенного шаблона переносятся в KReportDisplayParam (merge-семантика).
    if (KReportDisplayParam *dp = m_ctx.DisplayParam())
        dp->AppendItemParam(sub.m_mapItemConfigs);   // реф. @0x506ef0

    int total = 1;   // реф. стартует с 1 (сам элемент)
    for (const KReportTemplateItem &child : sub.m_lstItems)
        total += m_ctx.CreateItem(child, params);

    if (total > 0)
        AppendValid(Resolve(item.m_strID, params));
    return total;
}
