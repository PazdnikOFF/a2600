#include "report/KRTSimpleDisplay.h"

#include "report/KRTSimpleCreatorContext.h"

KRTSimpleDisplay::KRTSimpleDisplay(KRTAbsDataSource *pDataSource)
    : KRTAbsDisplay(pDataSource)
{
    // Реф. ctor @0x50a2b8: база → зануление +0xE0 → InitMembers().
    InitMembers();
}

KRTSimpleDisplay::~KRTSimpleDisplay() = default;

void KRTSimpleDisplay::InitMembers()
{
    // Реф. @0x50a268: new KRTSimpleCreatorContext(m_pDataSource, &m_displayParam).
    m_pContext = std::make_unique<KRTSimpleCreatorContext>(m_pDataSource, &m_displayParam);
}

bool KRTSimpleDisplay::Reset()
{
    m_displayParam.Reset();   // реф. @0x50a248 — и больше ничего
    return true;
}

bool KRTSimpleDisplay::Display(const KReportTemplateDataNew &data)
{
    // Реф. @0x50a308.
    m_displayParam.UpdateTemplateDisplayParam(data.m_mapConfigs, data.m_mapItemConfigs);
    // ⚠️ Реф. заводит ЛОКАЛЬНУЮ мапу params и НИ РАЗУ её не заполняет — в CreateItem
    // на каждый элемент уходит ПУСТАЯ мапа. Воспроизведено дословно.
    const std::map<std::string, std::string> params;
    for (const KReportTemplateItem &item : data.m_lstItems) {
        if (m_pContext)
            m_pContext->CreateItem(item, params);   // результат реф. не агрегирует
    }
    return true;
}
