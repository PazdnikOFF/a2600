#include "report/KReportTemplateCommonDef.h"

#include <map>

namespace report_template {

const std::string STR_PATH_SEPARATOR = "/";

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

} // namespace report_template
