#include "report/KRTSimpleCreatorContext.h"

#include "report/KRTAbsDataSource.h"
#include "report/KReportDisplayParam.h"
#include "report/KReportTemplateCommonDef.h"

namespace {
// Ключи реестра — строковые литералы из .rodata (0x865a70–0x865b40), которые
// статически конструирует _GLOBAL__sub_I_KRTSimpleCreatorContext.cpp @0x248f70.
const char *kKeyText       = "RT_TEXT_BLOCK";
const char *kKeyTextGroup  = "RT_TEXTGROUP_BLOCK";
const char *kKeyImage      = "RT_IMAGE_BLOCK";
const char *kKeyImageGroup = "RT_IMAGEGROUP_BLOCK";
const char *kKeyTable      = "RT_TABLE_BLOCK";
const char *kKeyTitleTable = "RT_TITLE_TABLE_BLOCK";
const char *kKeySubData    = "RT_SUB_DATA_BLOCK";
const char *kKeyRowTable   = "RT_ROW_TABLE_BLOCK";
const char *kKeyObZScore   = "RT_OB_Z_SCORE_BLOCK";

// Литералы из CreateItem.
const char *kRepeatCommand = "RepeatCommand";   // @0x865c98
const char *kRefColumnID   = "RefColumnID";     // @0x865c88 — разделитель для RevertPathByID
}   // namespace

KRTSimpleCreatorContext::KRTSimpleCreatorContext(KRTAbsDataSource *pDataSource,
                                                 KReportDisplayParam *pDisplayParam)
    : m_pDataSource(pDataSource), m_pDisplayParam(pDisplayParam)
{
    InitCreator();
}

KRTSimpleCreatorContext::~KRTSimpleCreatorContext() = default;

void KRTSimpleCreatorContext::InitCreator()
{
    // Реф. @0x508160 — девять регистраций. Четыре ключа таблицы получают ЧЕТЫРЕ
    // ОТДЕЛЬНЫХ объекта KRTSimpleTableItemCreator (в реф. это 4 разных operator new);
    // в отличие от «взрослого» KRTTableItemCreator, Simple-вариант НЕ принимает
    // строку подтипа — все четыре ключа ведут себя одинаково.
    m_mapCreators[kKeyText]       = std::make_unique<KRTSimpleTextItemCreator>(*this);
    m_mapCreators[kKeyTextGroup]  = std::make_unique<KRTSimpleTextGroupCreator>(*this);
    m_mapCreators[kKeyImage]      = std::make_unique<KRTSimpleImageItemCreator>(*this);
    m_mapCreators[kKeyImageGroup] = std::make_unique<KRTSimpleImageGroupCreator>(*this);
    m_mapCreators[kKeyTable]      = std::make_unique<KRTSimpleTableItemCreator>(*this);
    m_mapCreators[kKeyTitleTable] = std::make_unique<KRTSimpleTableItemCreator>(*this);
    m_mapCreators[kKeySubData]    = std::make_unique<KRTSimpleSubDataItemCreator>(*this);
    m_mapCreators[kKeyRowTable]   = std::make_unique<KRTSimpleTableItemCreator>(*this);
    m_mapCreators[kKeyObZScore]   = std::make_unique<KRTSimpleTableItemCreator>(*this);
}

int KRTSimpleCreatorContext::CreateItem(const KReportTemplateItem &item,
                                        const std::map<std::string, std::string> &params)
{
    // Реф. @0x509350.
    std::list<std::pair<std::string, std::vector<std::string>>> repeats;

    if (!m_pDisplayParam)
        return HandleRepeat(item, params, repeats);

    const auto &itemConfigs = m_pDisplayParam->ItemConfigs();
    auto cfg = itemConfigs.find(item.m_strID);       // ключ поиска — item +0x00 (m_strID)
    if (cfg == itemConfigs.end())
        return HandleRepeat(item, params, repeats);

    const auto &attrs = cfg->second.m_mapAttrs;
    auto rc = attrs.find(kRepeatCommand);
    if (rc == attrs.end())
        return HandleRepeat(item, params, repeats);

    // Реф.: RevertPathByID(значение "RepeatCommand", "RefColumnID") — разбор команды
    // повтора по литералу-разделителю. Результат самой команды дальше не используется
    // напрямую: перечень измерений берётся из атрибутов того же конфига (ниже).
    const std::vector<std::string> parts =
        report_template::RevertPathByID(rc->second, kRefColumnID);
    (void)parts;

    // Для каждого атрибута конфига, которого ЕЩЁ НЕТ во входящем params, спрашиваем у
    // источника данных перечень вариантов. ⚠️ Получатель вызова — именно ИСТОЧНИК
    // ДАННЫХ (`ldr x4,[x28,#8]` @0x509534), а слот +0x20 его vtable — это
    // GetTextOptional(fieldId, key, out); первым аргументом идёт item+0x80 = m_strDataSrc
    // (`add x1, x25, #0x80` @0x509548). Сверено дизасмом отдельно.
    for (const auto &kv : attrs) {
        if (params.count(kv.first))
            continue;
        std::vector<std::string> values;
        if (m_pDataSource && m_pDataSource->GetTextOptional(item.m_strDataSrc, kv.first, values)
            && !values.empty())
            repeats.emplace_back(kv.first, values);
    }
    return HandleRepeat(item, params, repeats);
}

int KRTSimpleCreatorContext::HandleRepeat(
    const KReportTemplateItem &item,
    const std::map<std::string, std::string> &params,
    const std::list<std::pair<std::string, std::vector<std::string>>> &repeats)
{
    // Реф. @0x508c00.
    if (repeats.empty()) {
        // База рекурсии: творец ищется по item +0x60 = m_strType.
        auto it = m_mapCreators.find(item.m_strType);
        if (it == m_mapCreators.end())
            return 0;                       // реф. возвращает 0, если типа нет в реестре
        return it->second->CreateItem(item, params);
    }

    // Рекурсия: снимаем ПЕРВОЕ измерение и перебираем все его варианты.
    auto rest = repeats;
    const auto head = rest.front();
    rest.pop_front();

    int total = 0;
    std::map<std::string, std::string> local = params;
    for (const std::string &v : head.second) {
        local[head.first] = v;              // insert-or-assign
        total += HandleRepeat(item, local, rest);
    }
    return total;                           // реф. СУММИРУЕТ результаты
}
