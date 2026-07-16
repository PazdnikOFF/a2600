#include "report/KReportTemplateCommonDef.h"
#include "report/KMeaStringUtil.h"

#include <map>
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

} // namespace report_template
