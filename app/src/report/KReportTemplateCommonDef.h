#pragma once

#include "report/KReportTemplateData.h"

#include <list>
#include <string>
#include <vector>

// Свободные функции namespace report_template (реф. KReportTemplateComonDef.cpp,
// опечатка «Comon» — как в оригинале). Разделитель ID везде — "/" (в реф. это
// параметр функций, а не хардкод; фактически всегда STR_PATH_SEPARATOR).
namespace report_template {

extern const std::string STR_PATH_SEPARATOR;   // "/"

// JOIN: ["A","B","C"] + "/" → "A/B/C". Пустой вектор → "". Ведущего разделителя нет.
std::string GenerateIDByPath(const std::vector<std::string> &path, const std::string &sep);

// SPLIT: "A/B/C" → ["A","B","C"]. Пустые токены в начале/середине СОХРАНЯЮТСЯ
// ("/A" → ["","A"]), хвостового пустого токена НЕТ ("A/" → ["A"]), "" → [].
std::vector<std::string> RevertPathByID(const std::string &id, const std::string &sep);

// Split → отбросить последний элемент → join. "/A/B" → "/A".
bool GetParentItemID(const std::string &id, std::string &out);

// ID прямых детей; при bRecursive — всех потомков, pre-order DFS.
// СОБСТВЕННЫЙ ID элемента НЕ включается (реф.).
bool GetSubItemsID(const KReportTemplateItem &item, std::list<std::string> &out,
                   bool bRecursive);

// Слияние деревьев. Дедуп ПО m_strName и НА КАЖДОМ УРОВНЕ ОТДЕЛЬНО (не по m_strID):
//   * имя найдено в dst → рекурсия ТОЛЬКО в под-элементы; собственные поля
//     dst-элемента НЕ трогаются (dst побеждает); в out ничего не кладётся;
//   * имя не найдено → элемент клонируется ДОСЛОВНО (включая m_strID — путь НЕ
//     перестраивается) и push_back в КОНЕЦ; в out идёт его ID + ID всех потомков.
bool MergeSubItem(std::list<KReportTemplateItem> &dst, const std::list<KReportTemplateItem> &src,
                  std::list<std::string> &out);

// Слияние данных шаблона, три фазы (реф. порядок):
//   1) m_mapConfigs      — dst[k] = v для всех k из src (SRC ПОБЕЖДАЕТ);
//   2) m_lstItems        — через MergeSubItem (DST побеждает, см. выше);
//   3) m_mapItemConfigs  — dst[k] = cfg для всех k из src (SRC ПОБЕЖДАЕТ, полная замена).
bool MergeData(KReportTemplateDataNew &dst, const KReportTemplateDataNew &src,
               std::list<std::string> &outIDs);

// Заголовок элемента для показа (реф. QueryTemplateItemRealTitle @0x595688). Точная
// логика резолва из дизасма не декодирована — принято: возврат m_strTitle элемента.
std::string QueryTemplateItemRealTitle(const KReportTemplateItem &item);

} // namespace report_template
