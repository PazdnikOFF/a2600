#include "KDocumentGenerator.h"

namespace {

// Константы реф. живут в namespace report_template как file-static std::string
// (по копии на TU). У нас их в KReportTemplateCommonDef.h нет, поэтому объявлены
// здесь с реф.-именами. ЗНАЧЕНИЯ сверены не с дизасмом, а с РЕАЛЬНЫМИ шаблонами
// прошивки (system/presetdata/syspreset/mainapp/patient/report/template/**.xml) —
// это более сильное доказательство, чем чтение статического инициализатора:
//   Section  — 40 вхождений, значения ровно три: "Body" / "Footer" / "Header"
//              (совпадает с тройкой символов STR_RT_VALUE_BODY/FOOTER/HEADER);
//   RefColumn — 14 вхождений.
const std::string STR_RT_ITEM_ATTR_SECTION   = "Section";
const std::string STR_RT_ITEM_ATTR_REFCOLUMN = "RefColumn";
const std::string STR_RT_VALUE_FOOTER        = "Footer";

} // namespace

KDocumentGenerator::KDocumentGenerator(KReportTemplateDataNew *pData)
    : m_pData(pData)
{
    // Реф.: m_pDoc=0; m_pData=p; m_strCurItemId=STR_INVALID_ITEM_ID;
    // strh wzr,[this,#0x38] — оба bool-флага обнуляются одной инструкцией;
    // m_pContext = new KRTCreatorContext(p) (0x40 байт).
    // У нас: контекст не создаётся (KRTCreatorContext — итерация 2),
    // m_strCurItemId пуст (значение STR_INVALID_ITEM_ID не восстановлено, см. .h).
}

std::string KDocumentGenerator::itemAttr(const std::string &id,
                                         const std::string &attr) const
{
    if (!m_pData)
        return std::string();
    auto itCfg = m_pData->m_mapItemConfigs.find(id);
    if (itCfg == m_pData->m_mapItemConfigs.end())
        return std::string();
    auto itAttr = itCfg->second.m_mapAttrs.find(attr);
    if (itAttr == itCfg->second.m_mapAttrs.end())
        return std::string();
    return itAttr->second;
}

void KDocumentGenerator::Save(KReportTemplateDataNew &out) const
{
    // Реф. @0x541628: присваивание map doc-атрибутов + глубокая копия списка
    // элементов (в реф. — через 7 string::swap + _List_node_base::swap на узел;
    // у нас эквивалент даёт обычное присваивание std::list).
    // ВАЖНО: реф. копирует ТОЛЬКО m_mapConfigs и m_lstItems. Карта m_mapItemConfigs
    // в Save НЕ участвует — out сохраняет свою (сверено по трём копируемым
    // контейнерам в дизасме мёртвого близнеца и по отсутствию обращения к +0x48).
    if (!m_pData)
        return;
    out.m_mapConfigs = m_pData->m_mapConfigs;
    out.m_lstItems   = m_pData->m_lstItems;
}

void KDocumentGenerator::ChangeCalcApps(const std::string &value)
{
    // Реф. @0x53d340: docAttrs["CalcApp"] = value.
    // Ключ "CalcApp" — литерал реф. (0x865c58, длина 7).
    if (!m_pData)
        return;
    m_pData->m_mapConfigs["CalcApp"] = value;
}

bool KDocumentGenerator::SetLayoutParam(const std::string &id,
                                        const std::string &value)
{
    // Реф. @0x53c5f0: в конфиг элемента пишется атрибут STR_RT_ITEM_ATTR_REFCOLUMN.
    // Реф. ВСЕГДА возвращает 1 (в мёртвом близнеце — то же поведение).
    if (!m_pData)
        return true;
    m_pData->m_mapItemConfigs[id].m_mapAttrs[STR_RT_ITEM_ATTR_REFCOLUMN] = value;
    return true;
}

bool KDocumentGenerator::HasFooterTemplateItem() const
{
    // Реф. @0x53c680: обход m_pData->m_lstItems (список @+0x30), для каждого элемента
    // конфиг по его id, сравнение атрибута Section с "Footer" (в дизасме — memcmp,
    // т.е. обычное сравнение std::string).
    if (!m_pData)
        return false;
    for (const KReportTemplateItem &item : m_pData->m_lstItems) {
        if (itemAttr(item.m_strID, STR_RT_ITEM_ATTR_SECTION) == STR_RT_VALUE_FOOTER)
            return true;
    }
    return false;
}

std::string KDocumentGenerator::FindItmeIdofPreFooter() const
{
    // Реф. @0x53d210 (опечатка "Itme" — реф., сохранена 1:1): id последнего элемента
    // ПЕРЕД первым с Section=="Footer". Обход того же списка верхнего уровня.
    // Если футера нет или он самый первый — результат пуст.
    if (!m_pData)
        return std::string();
    std::string prev;
    for (const KReportTemplateItem &item : m_pData->m_lstItems) {
        if (itemAttr(item.m_strID, STR_RT_ITEM_ATTR_SECTION) == STR_RT_VALUE_FOOTER)
            return prev;
        prev = item.m_strID;
    }
    return std::string();
}
