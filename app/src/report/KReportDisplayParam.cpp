#include "report/KReportDisplayParam.h"
#include "report/KReportTemplate.h"
#include "report/KReportDataSource.h"

void KReportDisplayParam::Reset()
{
    // Реф. @0x506c10 чистит ВСЁ состояние (оба map, оба set и флаг), а не только
    // набор валидных. Заодно сбрасываем и наш упрощённый слой.
    valid_.clear();
    ref_.clear();
    m_mapTemplateConfig.clear();
    m_mapItemConfig.clear();
    m_setValidItems.clear();
    m_setRefValidItems.clear();
    m_bHasRefValidItems = false;
}

// ═══════════════════ Референсный слой (сверено дизасмом) ═══════════════════

void KReportDisplayParam::AppendValidItem(const std::string &name)
{
    // Реф. @0x506ca0: обычная unique-вставка в набор валидных. Гейта по ref-множеству
    // НЕТ (проверено: во всей функции нет обращений к +0x08 и +0xa0), возврата тоже нет.
    m_setValidItems.insert(name);
}

bool KReportDisplayParam::IsItemValid(const std::string &name) const
{
    // Реф. @0x506a38: `ldrb w22,[x0,#8]; cbnz w22, <искать>; mov w22,#1`.
    // Флаг не взведён → true БЕЗУСЛОВНО; взведён → поиск в РЕФЕРЕНСНОМ множестве
    // (+0xa0), а не в том, что наполняет AppendValidItem.
    if (!m_bHasRefValidItems)
        return true;
    return m_setRefValidItems.find(name) != m_setRefValidItems.end();
}

void KReportDisplayParam::SetRefValidItems(const std::set<std::string> &items)
{
    // Реф. @0x506ec0: присваивание + безусловный взвод флага (`mov w0,#1; strb w0,[x19,#8]`).
    m_setRefValidItems = items;
    m_bHasRefValidItems = true;
}

void KReportDisplayParam::AppendItemParam(
    const std::map<std::string, KReportTemplateItemConfig> &params)
{
    // Реф. @0x506ef0: MERGE — существующий ключ ПРОПУСКАЕТСЯ, а не перезаписывается
    // (вставка идёт через _M_get_insert_hint_unique_pos, при занятом ключе — next).
    for (const auto &kv : params)
        m_mapItemConfig.insert(kv);
}

void KReportDisplayParam::UpdateTemplateDisplayParam(
    const std::map<std::string, std::string> &configs,
    const std::map<std::string, KReportTemplateItemConfig> &itemConfigs)
{
    // Реф. @0x507110 — РОВНО два `operator=`, и больше ничего: ни разбора ключей,
    // ни разделителей, ни пересчёта набора валидных. Замещение, а не merge.
    m_mapTemplateConfig = configs;
    m_mapItemConfig = itemConfigs;
}

bool KReportDisplayParam::AppendValidItem(const QString &name)
{
    if (name.isEmpty())
        return false;
    // реф.: если задано эталонное множество, элемент должен в него входить.
    if (!ref_.isEmpty() && !ref_.contains(name))
        return false;
    valid_.insert(name);
    return true;
}

bool KReportDisplayParam::IsItemValid(const QString &name) const
{
    return valid_.contains(name);
}

bool KReportDisplayParam::markItem(const ReportItem &it, const KReportDataSource &ds)
{
    // Сам элемент валиден, если у него есть непустое связанное значение.
    bool self = false;
    if (!it.dataSrc.isEmpty()) {
        const QString v = ds.GetValue(it.dataSource(), it.dataField());
        self = !v.isEmpty();
    }
    // Потомки: валиден, если валиден хотя бы один.
    bool anyChild = false;
    for (const ReportItem &c : it.children)
        if (markItem(c, ds))
            anyChild = true;

    const bool valid = self || anyChild;
    if (valid)
        AppendValidItem(it.name);
    return valid;
}

int KReportDisplayParam::UpdateTemplateDisplayParam(const QVector<ReportItem> &items,
                                                    const KReportDataSource &ds)
{
    Reset();
    for (const ReportItem &it : items)
        markItem(it, ds);
    return valid_.size();
}
