#include "KDocumentGenerator.h"

#include "KReportTemplateCommonDef.h"

#include <iterator>

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
// Значение сверено в .rodata (0x865d18: "CellAt\0\0"); в поставляемых шаблонах
// атрибут CellAt не встречается — на реальных данных cell-закрепления нет.
const std::string STR_RT_ITEM_ATTR_CELLAT    = "CellAt";

// Раскладка галереи снимков RT_IMAGE_TEXT_MAP: колонки MAP1/MAP2/… «зеркалят»
// эталонную MAP0 через атрибут SynColumnID. Имена вендорские, сверены с поставкой
// (system/presetdata/.../report/template/SubContent/*.xml) И с дизасмом статических
// инициализаторов (адреса .bss a97938/a97958/a97978/a97998).
//   "SynColumnID" — 92 вхождения (орфография вендора: SYN, НЕ SYNC). Значение — id
//   колонки MAP0, которую данная колонка зеркалит.
// ⚠️ ВНИМАНИЕ: needle-глобал реф. это "/RT_IMAGE_TEXT_MAP" СО СЛЭШЕМ, а голое
// "RT_IMAGE_TEXT_MAP" — ДРУГОЙ глобал (STR_TOP_IMAGE_TEXT_NAME, имя узла). Для реальных
// ключей конфигов (все начинаются с "/RT_IMAGE_TEXT_MAP") подстрочный тест совпадает,
// но литерал храним точный.
const std::string STR_REF_IMAGE_TEXT_MAP      = "/RT_IMAGE_TEXT_MAP";
const std::string STR_REF_IMAGE_TEXT_MAP0     = "/RT_IMAGE_TEXT_MAP/RT_IMAGE_TEXT_MAP0";
const std::string STR_REF_IMAGE_TEXT_MAP_EXT  = "/RT_MAIN_CONTENT/RT_IMAGE_TEXT_MAP";
const std::string STR_REF_IMAGE_TEXT_MAP0_EXT =
    "/RT_MAIN_CONTENT/RT_IMAGE_TEXT_MAP/RT_IMAGE_TEXT_MAP0";
const std::string STR_RT_ITEM_ATTR_SYNCOLUMNID = "SynColumnID";

} // namespace

const std::string &KDocumentGenerator::InvalidItemId()
{
    // Реф. STR_INVALID_ITEM_ID = "Invalid ID" (декомпилятор, см. .h).
    static const std::string kInvalid = "Invalid ID";
    return kInvalid;
}

KDocumentGenerator::KDocumentGenerator(KReportTemplateDataNew *pData)
    : m_pData(pData), m_strCurItemId(InvalidItemId())
{
    // Реф.: m_pDoc=0; m_pData=p; m_strCurItemId=STR_INVALID_ITEM_ID ("Invalid ID");
    // strh wzr,[this,#0x38] — оба bool-флага обнуляются одной инструкцией;
    // m_pContext = new KRTCreatorContext(p) (0x40 байт).
    // У нас: контекст не создаётся (KRTCreatorContext — итерация 2).
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

bool KDocumentGenerator::isCellPinned(const std::string &id) const
{
    // Реф. UpdateMovableFlag: конфиг элемента существует И имеет атрибут "CellAt".
    if (!m_pData)
        return false;
    auto it = m_pData->m_mapItemConfigs.find(id);
    if (it == m_pData->m_mapItemConfigs.end())
        return false;
    return it->second.m_mapAttrs.count(STR_RT_ITEM_ATTR_CELLAT) != 0;
}

const std::list<KReportTemplateItem> *
KDocumentGenerator::siblingsOf(const std::string &id) const
{
    // Реф.: parentPath = id.substr(0, find_last_of("/")); при отсутствии "/"
    // find_last_of даёт npos, реф. клампит к длине → parentPath = весь id.
    // FindRefItem(parentPath): нашёлся → его дети (+0xe0); не нашёлся → корень (+0x30).
    if (!m_pData)
        return nullptr;
    const std::size_t pos = id.find_last_of('/');
    const std::string parentPath = id.substr(0, pos == std::string::npos ? id.size() : pos);
    const KReportTemplateItem *parent = report_template::FindConstRefItem(*m_pData, parentPath);
    return parent ? &parent->m_lstSubItems : &m_pData->m_lstItems;
}

std::list<std::string>
KDocumentGenerator::GetAllItemIDs(const std::string &id,
                                 const KReportTemplateDataNew &data) const
{
    // Реф. @0x53fdb0 (декомпилятор + сверка статических инициализаторов). Порядок 1:1.
    std::list<std::string> result;

    // Нормализация id колонки MAP0 к id её контейнера (реф. присваивает cur один из двух
    // глобалов — relative или MAIN_CONTENT-prefixed; ветка A проверяется первой).
    std::string cur = id;
    if (id == STR_REF_IMAGE_TEXT_MAP0)
        cur = STR_REF_IMAGE_TEXT_MAP;
    else if (id == STR_REF_IMAGE_TEXT_MAP0_EXT)
        cur = STR_REF_IMAGE_TEXT_MAP_EXT;

    // Не image-text-map id → результат ровно [cur], map не обходится (реф. ранний возврат).
    if (cur.find(STR_REF_IMAGE_TEXT_MAP) == std::string::npos) {
        result.push_back(cur);
        return result;
    }

    // Собрать конфиги колонок галереи (ключ содержит "/RT_IMAGE_TEXT_MAP") во временный map
    // (реф. — отдельный проход, отсюда порядок сортировки ключей на выходе).
    std::map<std::string, KReportTemplateItemConfig> tmp;
    for (const auto &kv : data.m_mapItemConfigs) {
        if (kv.first.find(STR_REF_IMAGE_TEXT_MAP) != std::string::npos)
            tmp.insert(kv);
    }
    // Колонки, зеркалящие cur (attrs["SynColumnID"] == cur) — их id, в порядке ключа map.
    for (const auto &kv : tmp) {
        auto itSyn = kv.second.m_mapAttrs.find(STR_RT_ITEM_ATTR_SYNCOLUMNID);
        if (itSyn != kv.second.m_mapAttrs.end() && itSyn->second == cur)
            result.push_back(kv.first);
    }
    // И сам cur — ОДНИМ безусловным push, последним.
    result.push_back(cur);
    return result;
}

void KDocumentGenerator::SyncImageItemContent(const KReportTemplateItem &proto,
                                              KReportTemplateItem &target) const
{
    // Реф. @0x53c490 (декомпилятор): оставить в target.m_lstSubItems только те
    // суб-элементы, чьё m_strName ЕСТЬ среди суб-элементов proto. Фильтр по ИМЕНИ
    // (реф. читает node payload +0x20 = m_strName), НЕ по id — легко перепутать.
    // this в теле реф. не используется (метод по сути статический).
    for (auto it = target.m_lstSubItems.begin(); it != target.m_lstSubItems.end();) {
        bool keep = false;
        for (const KReportTemplateItem &p : proto.m_lstSubItems) {
            if (p.m_strName == it->m_strName) {
                keep = true;
                break;
            }
        }
        if (keep)
            ++it;
        else
            it = target.m_lstSubItems.erase(it);
    }
}

void KDocumentGenerator::SyncImageItemParam(const KReportTemplateDataNew &libData)
{
    // Реф. @0x53eb20 (декомпилятор): синхронизировать конфиги колонок-зеркал из libData
    // в наш m_pData->m_mapItemConfigs. Реф. логирует "[info] … SyncImageItemParam" —
    // лог опущен (off-device).
    if (!m_pData)
        return;

    // Плоский сбор конфигов суб-элементов image-text-map (реф. GetSubItemsParam:
    // ключ конфига СОДЕРЖИТ подстроку "RT_IMAGE_TEXT_MAP").
    std::map<std::string, KReportTemplateItemConfig> tmp;
    report_template::GetSubItemsParam(libData, STR_REF_IMAGE_TEXT_MAP, tmp);

    for (const auto &kv : tmp) {
        const std::string &itemId = kv.first;
        const KReportTemplateItemConfig &cfg = kv.second;

        // Конфиг без SynColumnID — не колонка-зеркало, пропускаем.
        auto itSyn = cfg.m_mapAttrs.find(STR_RT_ITEM_ATTR_SYNCOLUMNID);
        if (itSyn == cfg.m_mapAttrs.end())
            continue;
        const std::string &synVal = itSyn->second;

        if (m_pData->m_mapItemConfigs.find(synVal) == m_pData->m_mapItemConfigs.end()) {
            // Колонка-эталон (значение SynColumnID) в наших данных отсутствует —
            // запись по этому itemId устарела, стираем (реф. equal_range+erase; ключ
            // map уникален → эквивалентно erase(itemId)).
            m_pData->m_mapItemConfigs.erase(itemId);
        } else {
            // Эталон есть — переносим конфиг зеркала под ключом itemId.
            KReportTemplateItemConfig &dst = m_pData->m_mapItemConfigs[itemId];
            dst.m_bUserDefine = cfg.m_bUserDefine;
            dst.m_strName     = cfg.m_strName;
            dst.m_mapAttrs    = cfg.m_mapAttrs;
        }
    }
}

void KDocumentGenerator::UpdateMovableFlag(const std::string &id)
{
    // Реф. @0x53c7b0 (восстановлено декомпилятором Ghidra). Порядок 1:1.
    m_bCanMoveFront = false;
    m_bCanMoveBack = false;

    if (id == InvalidItemId())
        return;
    // Сам элемент cell-закреплён → не двигается вообще.
    if (isCellPinned(id))
        return;

    const std::list<KReportTemplateItem> *sibs = siblingsOf(id);
    if (!sibs)
        return;

    // Ищем элемент среди соседей, запоминая предыдущего и следующего.
    for (auto it = sibs->begin(); it != sibs->end(); ++it) {
        if (it->m_strID != id)
            continue;
        // Предыдущий сосед → можно ли двигать вперёд.
        if (it != sibs->begin()) {
            auto prev = std::prev(it);
            m_bCanMoveFront = !isCellPinned(prev->m_strID);
        }
        // Следующий сосед → можно ли двигать назад.
        auto nxt = std::next(it);
        if (nxt != sibs->end())
            m_bCanMoveBack = !isCellPinned(nxt->m_strID);
        return;
    }
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
