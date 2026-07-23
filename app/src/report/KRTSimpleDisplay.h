#pragma once

#include <memory>

#include "report/KReportDisplayParam.h"
#include "report/KReportTemplateData.h"

class KRTAbsDataSource;
class KRTSimpleCreatorContext;

// Базовый класс движков отображения отчёта (реф. KRTAbsDisplay, ctor @0x506928).
// Держит указатель на источник данных (+0x08). Подробный API реф. базы в этой
// итерации НЕ реверсирован — портирован минимум, нужный Simple-движку.
class KRTAbsDisplay
{
public:
    explicit KRTAbsDisplay(KRTAbsDataSource *pDataSource) : m_pDataSource(pDataSource) {}
    virtual ~KRTAbsDisplay() = default;

    virtual bool Reset() = 0;
    virtual bool Display(const KReportTemplateDataNew &data) = 0;

    KRTAbsDataSource *DataSource() const { return m_pDataSource; }

protected:
    KRTAbsDataSource *m_pDataSource = nullptr;   // +0x08
};

// «Простой» движок отображения отчёта (реф. KRTSimpleDisplay : KRTAbsDisplay,
// ctor @0x50a2b8, sizeof 0xE8).
//
// Layout реф.: база (vptr + m_pDataSource@+0x08), затем ВСТРОЕННЫЙ ПО ЗНАЧЕНИЮ
// `KReportDisplayParam` (+0x10, ровно 208 = 0xD0 байт — сходится с его sizeof),
// затем `KRTSimpleCreatorContext*` (+0xE0, владеющий).
// Контекст получает УКАЗАТЕЛЬ на встроенный параметр — он им не владеет.
class KRTSimpleDisplay : public KRTAbsDisplay
{
public:
    explicit KRTSimpleDisplay(KRTAbsDataSource *pDataSource);
    ~KRTSimpleDisplay() override;

    void InitMembers();                 // реф. @0x50a268
    bool Reset() override;              // реф. @0x50a248 — сброс параметра, всегда true

    // Реф. @0x50a308 — вся функция:
    //   1) m_displayParam.UpdateTemplateDisplayParam(data.m_mapConfigs, data.m_mapItemConfigs);
    //   2) для КАЖДОГО элемента data.m_lstItems → m_pContext->CreateItem(item, params),
    //      где `params` — ЛОКАЛЬНАЯ ПУСТАЯ мапа, которая НИКОГДА не заполняется;
    //   3) return true (результаты CreateItem не агрегируются).
    bool Display(const KReportTemplateDataNew &data) override;

    KReportDisplayParam &DisplayParam() { return m_displayParam; }
    KRTSimpleCreatorContext *Context() const { return m_pContext.get(); }

private:
    KReportDisplayParam m_displayParam;                    // +0x10 (по значению!)
    std::unique_ptr<KRTSimpleCreatorContext> m_pContext;   // +0xE0
};
