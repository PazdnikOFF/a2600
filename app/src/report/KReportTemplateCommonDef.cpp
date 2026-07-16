#include "report/KReportTemplateCommonDef.h"
#include "report/KMeaStringUtil.h"

#include <QObject>

#include <map>
#include <set>
#include <vector>

namespace report_template {

const std::string STR_PATH_SEPARATOR = "/";

namespace {
// Форматы сериализации (реф. .bss-глобалы, собираются конкатенацией в static-init).
const char *FMT_ATTR      = "%s|%s;";   // STR_ATTR_FORMAT — пара key|value;
const char *FMT_SOURCE_ID = "%s,%s";    // STR_SOURCE_ID_FORMAT — src,mapString
const char *SEP_PAIR      = ";";
const char *SEP_KV        = "|";
// Маркеры IsPatientInfoTitleBold (реф. литералы).
const char *MARK_PATIENT_INFO = "RT_PATIENT_INFO";
const char *MARK_HOSPITAL_OTHER = "HOSPITAL_OTHER";
// AppendCustomedItem (реф. CUSTOMTED_SECTION_TAG / STR_RT_ELEMENT_TITLE_TABLE_BLOCK).
const char *CUSTOMED_SECTION_TAG = "KW_NEW_SECTION";
const char *TYPE_TITLE_TABLE = "RT_TITLE_TABLE_BLOCK";
} // namespace

std::string GenerateIDByPath(const std::vector<std::string> &path, const std::string &sep)
{
    std::string out;
    for (size_t i = 0; i < path.size(); ++i) {
        if (i == 0)
            out = path[i];
        else
            out += sep + path[i];
    }
    return out;
}

std::vector<std::string> RevertPathByID(const std::string &id, const std::string &sep)
{
    std::vector<std::string> out;
    if (sep.empty())
        return out;
    size_t pos = 0;
    for (;;) {
        const size_t p = id.find(sep, pos);
        if (p == std::string::npos)
            break;
        out.push_back(id.substr(pos, p - pos));   // пустые токены сохраняются
        pos = p + sep.size();
    }
    // Хвостовой токен — только если что-то осталось (реф. cmp pos < size).
    if (pos < id.size())
        out.push_back(id.substr(pos));
    return out;
}

bool GetParentItemID(const std::string &id, std::string &out)
{
    std::vector<std::string> vec = RevertPathByID(id, STR_PATH_SEPARATOR);
    if (!vec.empty())
        vec.pop_back();
    out = GenerateIDByPath(vec, STR_PATH_SEPARATOR);
    return true;
}

bool GetSubItemsID(const KReportTemplateItem &item, std::list<std::string> &out, bool bRecursive)
{
    for (const KReportTemplateItem &c : item.m_lstSubItems) {
        out.push_back(c.m_strID);          // сначала ID ребёнка…
        if (bRecursive)
            GetSubItemsID(c, out, true);   // …затем его поддерево (pre-order)
    }
    return true;
}

bool MergeSubItem(std::list<KReportTemplateItem> &dst, const std::list<KReportTemplateItem> &src,
                  std::list<std::string> &out)
{
    // Индекс текущего уровня dst по m_strName (реф.: при дублях побеждает последний).
    std::map<std::string, KReportTemplateItem *> index;
    for (KReportTemplateItem &d : dst)
        index[d.m_strName] = &d;

    for (const KReportTemplateItem &s : src) {
        const auto it = index.find(s.m_strName);
        if (it != index.end()) {
            // Совпало имя: только рекурсия внутрь; поля dst не трогаем, в out ничего.
            MergeSubItem(it->second->m_lstSubItems, s.m_lstSubItems, out);
            continue;
        }
        // Новый элемент: клонируем дословно (m_strID переносится как есть).
        dst.push_back(s);
        out.push_back(s.m_strID);
        GetSubItemsID(s, out, true);
    }
    return true;
}

bool MergeData(KReportTemplateDataNew &dst, const KReportTemplateDataNew &src,
               std::list<std::string> &outIDs)
{
    for (const auto &kv : src.m_mapConfigs)
        dst.m_mapConfigs[kv.first] = kv.second;      // src побеждает

    MergeSubItem(dst.m_lstItems, src.m_lstItems, outIDs);

    for (const auto &kv : src.m_mapItemConfigs)
        dst.m_mapItemConfigs[kv.first] = kv.second;  // src побеждает, полная замена

    return true;
}

std::string ConvertMapToString(const std::map<std::string, std::string> &m)
{
    std::string result;   // стартует пустой
    KMeaStringUtil util;
    for (const auto &kv : m) {
        if (kv.first.empty())      // реф.: пары с пустым КЛЮЧОМ пропускаются
            continue;
        result += util.FormatStr(FMT_ATTR, kv.first.c_str(), kv.second.c_str());
    }
    return result;   // "key1|value1;key2|value2;" — завершающий ';' всегда
}

void ConvertStringToMap(const std::string &s, std::map<std::string, std::string> &out)
{
    // out НЕ очищается (merge). Split(";") → пары, Split("|") → key/value.
    KMeaStringUtil util;
    for (const std::string &pair : util.SplitStr(s, SEP_PAIR)) {
        if (pair.empty())
            continue;
        const std::vector<std::string> kv = util.SplitStr(pair, SEP_KV);
        // реф.: РОВНО 2 токена И непустое значение.
        if (kv.size() == 2 && !kv[1].empty())
            out[kv[0]] = kv[1];
    }
}

bool QueryTemplateItemRealTitle(const KReportTemplateItem &item, std::string &out)
{
    // реф. @0x595688: out ← m_strTitle (дефолт). Динамический резолв для 4 маркеров
    // m_strName (RT_RESERVED1/2, RT_CUSTOM_FIELD1/2_TITLE) идёт через device-only
    // конфиги (KPatientListConfigSetupHandler / KReportEditUIConfig) — off-device не
    // воспроизводим, потому всегда падаем в дефолт и возвращаем false.
    out = item.m_strTitle;
    return false;
}

bool ConvertToSourceID(const std::string &src,
                       const std::map<std::string, std::string> &param, std::string &out)
{
    // реф. @0x5954b0: out = src + "," + ConvertMapToString(param). Завершающая ',' всегда.
    const std::string mapStr = ConvertMapToString(param);
    KMeaStringUtil util;
    out = util.FormatStr(FMT_SOURCE_ID, src.c_str(), mapStr.c_str());
    return true;
}

std::string GenerateIDByString(const std::string &a, const std::string &b,
                               const std::string &sep)
{
    if (a.empty() || b.empty())
        return a + b;         // реф.: без sep, если любой конец пуст
    return a + sep + b;
}

bool IsPatientInfoTitleBold(const KReportTemplateItem &item)
{
    // реф. @0x5955e8: substring-поиск двух маркеров в m_strID.
    const std::string &id = item.m_strID;
    return id.find(MARK_PATIENT_INFO) != std::string::npos
        || id.find(MARK_HOSPITAL_OTHER) != std::string::npos;
}

namespace {
// Рекурсивная реализация коммит-спуска FindConstRefItem (реф. итеративная с переброской
// end-регистра на sentinel детей = отбрасывание сиблингов при уходе в поддерево).
const KReportTemplateItem *findIn(const std::list<KReportTemplateItem> &level,
                                  const std::string &id)
{
    for (const KReportTemplateItem &node : level) {
        if (node.m_strID == id)
            return &node;
        // node — предок id? тогда КОММИТИМСЯ в его детей (сиблинги не смотрим).
        if (id.find(node.m_strID + STR_PATH_SEPARATOR) != std::string::npos)
            return findIn(node.m_lstSubItems, id);
    }
    return nullptr;
}
} // namespace

const KReportTemplateItem *FindConstRefItem(const KReportTemplateDataNew &data,
                                            const std::string &id)
{
    return findIn(data.m_lstItems, id);
}

KReportTemplateItem *FindRefItem(KReportTemplateDataNew &data, const std::string &id)
{
    // реф. — хвостовой b в FindConstRefItem; non-const data сужается до const&.
    return const_cast<KReportTemplateItem *>(FindConstRefItem(data, id));
}

bool UpdateItemID(KReportTemplateItem &item, const std::string &parentId)
{
    // Pre-order: сперва свой ID, затем детям передаём НОВЫЙ item.m_strID.
    item.m_strID = GenerateIDByString(parentId, item.m_strName, STR_PATH_SEPARATOR);
    for (KReportTemplateItem &child : item.m_lstSubItems)
        UpdateItemID(child, item.m_strID);
    return true;
}

bool GetSubData(const KReportTemplateDataNew &data, const std::string &key,
                KReportTemplateDataNew &out)
{
    // реф. @0x595388 — скомпилированная заглушка `mov w0,#0; ret`. Логики нет.
    (void)data; (void)key; (void)out;
    return false;
}

bool HasSameNameInGroup(KReportTemplateDataNew &data, const std::string &id,
                        const std::string &name)
{
    // Родитель = id без последнего "/"-сегмента (find_last_of; miss → parent==id).
    const std::size_t p = id.find_last_of(STR_PATH_SEPARATOR);
    const std::string parent = id.substr(0, p);   // p==npos → весь id

    const std::list<KReportTemplateItem> *group = nullptr;
    if (parent.empty()) {
        group = &data.m_lstItems;                 // верхний уровень
    } else {
        KReportTemplateItem *pParent = FindRefItem(data, parent);
        if (!pParent)
            return false;                         // реф. — "not find p_ref_parent"
        group = &pParent->m_lstSubItems;
    }

    for (const KReportTemplateItem &sib : *group) {
        if (sib.m_strID == id)                    // сам себя не считаем
            continue;
        if (sib.m_strTitle == name)               // sic: сравнение с m_strTitle
            return true;
    }
    return false;
}

namespace {
// Рекурсивный сбор под-элементов (плоские поверхностные копии). Общая база двух overload
// GetSubItems. out НЕ очищается; сам родитель не включается; pre-order.
void collectSubItems(const std::list<KReportTemplateItem> &level,
                     std::list<KReportTemplateItem> &out, bool bRecursive)
{
    for (const KReportTemplateItem &child : level) {
        KReportTemplateItem copy = child;   // 7 строк…
        copy.m_lstSubItems.clear();         // …без под-дерева (реф. — плоский список)
        out.push_back(copy);
        if (bRecursive && !child.m_lstSubItems.empty())
            collectSubItems(child.m_lstSubItems, out, true);
    }
}
} // namespace

bool GetSubItems(const KReportTemplateItem &item, std::list<KReportTemplateItem> &out,
                 bool bRecursive)
{
    collectSubItems(item.m_lstSubItems, out, bRecursive);
    return true;
}

bool GetSubItems(const KReportTemplateDataNew &data, const std::string &id,
                 std::list<KReportTemplateItem> &out, bool bRecursive)
{
    if (id.empty()) {
        collectSubItems(data.m_lstItems, out, bRecursive);   // корень, без Find
        return true;
    }
    const KReportTemplateItem *p = FindConstRefItem(data, id);
    if (!p)
        return false;   // out не тронут
    collectSubItems(p->m_lstSubItems, out, bRecursive);
    return true;
}

bool RemoveSubItem(KReportTemplateDataNew &data, const std::string &parentId,
                   const std::string &id)
{
    KReportTemplateItem *parent = FindRefItem(data, parentId);   // без root-фолбэка
    if (!parent)
        return false;

    for (auto it = parent->m_lstSubItems.begin(); it != parent->m_lstSubItems.end(); ++it) {
        if (it->m_strName == id) {          // sic: сравнение с m_strName, не m_strID
            parent->m_lstSubItems.erase(it);
            // Чистка конфигов потомков: ключи, содержащие "<parentId>/<id>/".
            const std::string prefix =
                GenerateIDByString(parentId, id, STR_PATH_SEPARATOR) + STR_PATH_SEPARATOR;
            for (auto cit = data.m_mapItemConfigs.begin();
                 cit != data.m_mapItemConfigs.end(); ) {
                if (cit->first.find(prefix) != std::string::npos)
                    cit = data.m_mapItemConfigs.erase(cit);
                else
                    ++cit;
            }
            return true;
        }
    }
    return false;
}

bool AppendSubItem(KReportTemplateDataNew &data, const std::string &parentId,
                   const KReportTemplateItem &item)
{
    KReportTemplateItem *parent = FindRefItem(data, parentId);   // без root-фолбэка
    if (!parent)
        return false;

    std::set<std::string> names;
    for (const KReportTemplateItem &c : parent->m_lstSubItems)
        names.insert(c.m_strName);
    if (names.count(item.m_strName))
        return false;   // реф. — лог [APP][W] "AppenSubItem failed!" (device-only), отброшено

    parent->m_lstSubItems.push_back(item);   // глубокая дословная копия, m_strID НЕ пересчитан
    return true;
}

bool AppendSubItem(KReportTemplateDataNew &data, const std::string &parentId,
                   const std::list<KReportTemplateItem> &items)
{
    KReportTemplateItem *parent = FindRefItem(data, parentId);
    if (!parent)
        return false;

    // Множество имён строится ОДИН РАЗ и НЕ обновляется в цикле (реф.): внутрибатчевые
    // дубли обе добавляются, проверка — только против исходных детей.
    std::set<std::string> names;
    for (const KReportTemplateItem &c : parent->m_lstSubItems)
        names.insert(c.m_strName);

    for (const KReportTemplateItem &item : items) {
        if (names.count(item.m_strName))
            continue;                            // дубль с существующим → skip, не прерывая
        parent->m_lstSubItems.push_back(item);   // deep verbatim; names НЕ пополняется
    }
    return true;
}

bool GetCustomedSections(const KReportTemplateDataNew &data, std::vector<std::string> &out)
{
    out.clear();
    for (const KReportTemplateItem &item : data.m_lstItems) {   // только верхний уровень
        const auto it = data.m_mapItemConfigs.find(item.m_strID);
        if (it != data.m_mapItemConfigs.end() && it->second.m_bUserDefine)
            out.push_back(item.m_strID);
    }
    return true;
}

bool RenameCustomedItem(KReportTemplateDataNew &data, const std::string &id,
                        const std::string &newName)
{
    KReportTemplateItem *item = FindRefItem(data, id);
    if (!item)
        return false;                 // реф. — "not find rename ref item"
    item->m_strTitle = newName;       // sic: пишет m_strTitle, НЕ m_strName; без миграции ID
    return true;
}

bool DeleteCustomedItem(KReportTemplateDataNew &data, const std::string &parentId,
                        const KReportTemplateItem &item)
{
    if (!parentId.empty())            // реф. — только корень (parentId == "")
        return false;

    bool erased = false;
    for (auto it = data.m_lstItems.begin(); it != data.m_lstItems.end(); ++it) {
        if (it->m_strID == item.m_strID) {   // sic: матч по m_strID (в RemoveSubItem — m_strName)
            data.m_lstItems.erase(it);
            erased = true;
            break;
        }
    }
    if (!erased)
        return false;

    // Чистка конфигов: ключ == id ЛИБО «родитель ключа» СОДЕРЖИТ id (substring-баг сохранён).
    for (auto c = data.m_mapItemConfigs.begin(); c != data.m_mapItemConfigs.end(); ) {
        const std::string &key = c->first;
        const std::string parentOfKey = key.substr(0, key.find_last_of(STR_PATH_SEPARATOR));
        if (key == item.m_strID || parentOfKey.find(item.m_strID) != std::string::npos)
            c = data.m_mapItemConfigs.erase(c);
        else
            ++c;
    }
    return true;
}

bool AppendCustomedItem(KReportTemplateDataNew &data, const std::string &parentId,
                        KReportTemplateItem &item)
{
    if (!parentId.empty())            // реф. — только корень; иначе "not find parent item"
        return false;

    // Первое свободное имя KW_NEW_SECTION_<n> среди сиблингов верхнего уровня.
    std::set<std::string> names;
    for (const KReportTemplateItem &c : data.m_lstItems)
        names.insert(c.m_strName);

    KMeaStringUtil util;
    int n = 1;
    std::string candidate;
    for (;;) {
        candidate = util.FormatStr("%s_%d", CUSTOMED_SECTION_TAG, n);
        if (!names.count(candidate))
            break;
        ++n;
    }

    // Мутация item на месте.
    item.m_strName = candidate;
    // tr — device-only; off-device возвращает исходный тег → title = "KW_NEW_SECTION<n>".
    item.m_strTitle = util.FormatStr("%s%d",
        QObject::tr(CUSTOMED_SECTION_TAG).toUtf8().constData(), n);
    item.m_strID = parentId + STR_PATH_SEPARATOR + item.m_strName;   // безусловный "/"
    item.m_strType = TYPE_TITLE_TABLE;
    item.m_strShowTitle.insert(0, "1");   // реф. _M_replace(0,0,"1") — префикс
    item.m_strColumn.insert(0, "3");

    // Конфиг кастомной секции.
    KReportTemplateItemConfig cfg;
    cfg.m_bUserDefine = true;
    cfg.m_strName = item.m_strID;
    cfg.m_mapAttrs["UserDefine"]  = "1";
    cfg.m_mapAttrs["Append"]      = "1";
    cfg.m_mapAttrs["RefColumn"]   = "3";
    cfg.m_mapAttrs["FontType"]    = "ThirdTitle";
    cfg.m_mapAttrs["RefColumnID"] = item.m_strID;
    data.m_mapItemConfigs.insert({item.m_strID, cfg});   // без перезаписи, если ключ есть

    data.m_lstItems.push_back(item);   // копия в конец
    return true;
}

} // namespace report_template
