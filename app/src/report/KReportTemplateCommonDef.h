#pragma once

#include "report/KReportTemplateData.h"

#include <list>
#include <map>
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

// Сериализация map атрибутов в строку (реф. ConvertMapToString @0x595398). Формат
// "%s|%s;" на пару → "key1|value1;key2|value2;" (ЗАВЕРШАЮЩИЙ ';' всегда). Ключи с
// пустой строкой ПРОПУСКАЮТСЯ; порядок — сортировка std::map по ключу.
std::string ConvertMapToString(const std::map<std::string, std::string> &m);

// Обратный парсер (реф. ConvertStringToMap @0x597978). Split(";") → пары, каждую
// Split("|") → ключ/значение; берётся ТОЛЬКО пара из РОВНО 2 токенов с НЕПУСТЫМ
// значением. out НЕ очищается (merge-семантика). Точный инверс ConvertMapToString
// (пустые значения при round-trip теряются — как в реф.).
void ConvertStringToMap(const std::string &s, std::map<std::string, std::string> &out);

// Заголовок элемента для показа (реф. QueryTemplateItemRealTitle @0x595688). out ←
// m_strTitle (дефолт), return false. Динамические ветки для 4 маркеров m_strName
// (RT_RESERVED1/2, RT_CUSTOM_FIELD1/2_TITLE) резолвят заголовок через DEVICE-only
// конфиги (KPatientListConfigSetupHandler/KReportEditUIConfig) — off-device НЕ
// воспроизводимы, потому для всех элементов возвращаем m_strTitle + false.
bool QueryTemplateItemRealTitle(const KReportTemplateItem &item, std::string &out);

// Формирование source-id (реф. ConvertToSourceID @0x5954b0). out = src + "," +
// ConvertMapToString(param) (формат "%s,%s"; ЗАВЕРШАЮЩАЯ ',' всегда, даже при пустом
// param). Возвращает true. Это НЕ подстановка плейсхолдеров — просто конкатенация
// src с сериализованной map параметров.
bool ConvertToSourceID(const std::string &src,
                       const std::map<std::string, std::string> &param, std::string &out);

// JOIN двух строк через sep (реф. GenerateIDByString @0x595b78). Если a ИЛИ b пусты →
// a+b (без sep); иначе a+sep+b. Соответствует m_strID = parentPath + "/" + Name.
std::string GenerateIDByString(const std::string &a, const std::string &b,
                               const std::string &sep);

// Предикат «жирный заголовок инфо-о-пациенте» (реф. IsPatientInfoTitleBold @0x5955e8).
// true, если m_strID СОДЕРЖИТ "RT_PATIENT_INFO" ИЛИ "HOSPITAL_OTHER" (substring).
bool IsPatientInfoTitleBold(const KReportTemplateItem &item);

// Поиск элемента дерева по m_strID (реф. FindConstRefItem @0x595d60). Спуск ПО ПУТИ, не
// плоский обход: на уровне ищет точное совпадение m_strID==id; иначе, если id содержит
// "<node.m_strID>/" — КОММИТИТСЯ в поддерево node (сиблинги отбрасываются). nullptr, если
// уровень исчерпан. Указатель валиден, пока живо дерево.
const KReportTemplateItem *FindConstRefItem(const KReportTemplateDataNew &data,
                                            const std::string &id);
// Non-const близнец (реф. FindRefItem @0x595e68 — хвостовой b в FindConstRefItem).
KReportTemplateItem *FindRefItem(KReportTemplateDataNew &data, const std::string &id);

// Пересчёт m_strID поддерева от родительского ID (реф. UpdateItemID @0x595c70). Pre-order:
// item.m_strID = GenerateIDByString(parentId, item.m_strName, "/"); затем рекурсия в детей
// с НОВЫМ item.m_strID как их parentId. Возврат true.
bool UpdateItemID(KReportTemplateItem &item, const std::string &parentId);

// реф. GetSubData @0x595388 — В БИНАРНИКЕ ЭТО СКОМПИЛИРОВАННАЯ ЗАГЛУШКА (`return false`,
// out не трогается; сосед AppendSubData — такая же). Реальной логики нет — воспроизводим 1:1.
bool GetSubData(const KReportTemplateDataNew &data, const std::string &key,
                KReportTemplateDataNew &out);

// Проверка дубля в группе-сиблингах (реф. HasSameNameInGroup @0x561df0). ВНИМАНИЕ: несмотря
// на имя аргумента, сравнивается с m_strTitle сиблинга (НЕ m_strName!). Родитель = id без
// последнего "/"-сегмента (find_last_of); пустой → корень m_lstItems, иначе FindRefItem(parent)
// (miss → false). true, если у сиблинга с ДРУГИМ m_strID найден m_strTitle == name.
bool HasSameNameInGroup(KReportTemplateDataNew &data, const std::string &id,
                        const std::string &name);

} // namespace report_template
