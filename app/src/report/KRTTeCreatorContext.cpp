#include "report/KRTTeCreatorContext.h"

#include "report/KRTTeAbsItemCreator.h"

#include <QColor>

KRTTeCreatorContext::KRTTeCreatorContext(KRTAbsDataSource *pDataSource,
                                         KReportDisplayParam *pDisplayParam)
    : m_pDataSource(pDataSource), m_pDisplayParam(pDisplayParam)
{
    InitCreator();   // реф. ctor @0x50d3e8 наполняет реестр ОДИН РАЗ
}

KRTTeCreatorContext::~KRTTeCreatorContext() = default;

void KRTTeCreatorContext::InitCreator()
{
    // Реф. @0x50c9d0 — ровно семь регистраций; ключи — литералы @0x865a70..0x865b08.
    m_mapCreators["RT_TEXT_BLOCK"]        = std::make_unique<KRTTeTextItemCreator>(*this);
    m_mapCreators["RT_TEXTGROUP_BLOCK"]   = std::make_unique<KRTTeTextGroupCreator>(*this);
    m_mapCreators["RT_IMAGE_BLOCK"]       = std::make_unique<KRTTeImageItemCreator>(*this);
    m_mapCreators["RT_IMAGEGROUP_BLOCK"]  = std::make_unique<KRTTeImageGroupCreator>(*this);
    m_mapCreators["RT_TABLE_BLOCK"]       = std::make_unique<KRTTeTableItemCreator>(*this);
    m_mapCreators["RT_TITLE_TABLE_BLOCK"] = std::make_unique<KRTTeTableItemCreator>(*this);
    m_mapCreators["RT_SUB_DATA_BLOCK"]    = std::make_unique<KRTTeSubDataItemCreator>(*this);
    // RT_ROW_TABLE_BLOCK и RT_OB_Z_SCORE_BLOCK у Te НЕ регистрируются (в отличие от Simple).
}

QColor KRTTeCreatorContext::GetFontColor(const KReportTemplateItem *) const
{
    return m_defaultFontColor;   // реф. @0x50b918 — тело не реверсировано
}

bool KRTTeCreatorContext::IsGetFontChineseShow(const KReportTemplateItem *) const
{
    return false;                // реф. @0x50bba0 — тело не реверсировано
}

void KRTTeCreatorContext::Reset()
{
    // Реф. @0x50d5b8. Точный состав сбрасываемого состояния НЕ реверсирован.
    // ⚠️ Реестр творцов здесь чистить НЕЛЬЗЯ: в реф. он наполняется один раз
    // (InitCreator @0x50c9d0 из конструктора), а Reset вызывается перед КАЖДЫМ рендером
    // (см. OnReportPreview @0x4fdce8) — очистка сделала бы движок пустым со второго вызова.
    // Первая версия порта делала именно это, и отчёт выходил пустой страницей;
    // поймано скриншотом режима `reportpipeline`.
    // Сбрасывается состояние рендера (слияние заголовков и т.п.) — не реверсировано.
}

void KRTTeCreatorContext::RegisterCreator(const std::string &type,
                                          std::unique_ptr<KRTTeAbsItemCreator> c)
{
    m_mapCreators[type] = std::move(c);
}

int KRTTeCreatorContext::CreateItem(const KReportTemplateItem &item,
                                    const std::map<std::string, std::string> &cfgMap,
                                    QTextTableCell &cell)
{
    // Реф. @0x5102f8: поиск творца по типу блока (как в Simple-контексте) и вызов его
    // CreateItem. Раскрутка повторов у Te вынесена в отдельные HandleRepeat/HandleLrRepeat
    // (@0x50fb08 / @0x50e208) — они НЕ портированы в этой итерации.
    auto it = m_mapCreators.find(item.m_strType);
    if (it == m_mapCreators.end())
        return 0;
    return it->second->CreateItem(item, cfgMap, cell);
}

int KRTTeCreatorContext::GetFontSize(const KReportTemplateItem *) const
{
    return m_defaultFontSize;   // реф. @0x50bed8 — тело не реверсировано
}

void KRTTeCreatorContext::InsertSplitLine(const KSplitLineInfo &, QTextTable *)
{
    // Реф. @0x50c1c0 — не реверсировано.
}
