#include "KRTCreatorContext.h"

#include "KRTAbsItemCreator.h"

namespace report_template {
const std::string STR_RT_ELEMENT_TEXT_BLOCK        = "RT_TEXT_BLOCK";
const std::string STR_RT_ELEMENT_IMAGE_BLOCK       = "RT_IMAGE_BLOCK";
const std::string STR_RT_ELEMENT_IMAGEGROUP_BLOCK  = "RT_IMAGEGROUP_BLOCK";
const std::string STR_RT_ELEMENT_TABLE_BLOCK       = "RT_TABLE_BLOCK";
const std::string STR_RT_ELEMENT_TITLE_TABLE_BLOCK = "RT_TITLE_TABLE_BLOCK";
const std::string STR_RT_ELEMENT_SUB_DATA_BLOCK    = "RT_SUB_DATA_BLOCK";
} // namespace report_template

// --- KRTAbsItemCreator: базовые реализации (реф. — заглушки с printf) --------

bool KRTAbsItemCreator::CreateBlock(KReportTemplateItem *, QTextTableCell &)
{
    return true;   // реф.: printf(m_strType); return true;
}

bool KRTAbsItemCreator::CreateBlock(KReportTemplateItem *, QTextFrame *)
{
    return true;
}

bool KRTAbsItemCreator::UpdateBlock(KReportTemplateItem *, QTextFrame *)
{
    return true;
}

KRTTextItemCreator::KRTTextItemCreator(KRTCreatorContext &ctx)
    : KRTAbsItemCreator(report_template::STR_RT_ELEMENT_TEXT_BLOCK, ctx) {}

KRTImageItemCreator::KRTImageItemCreator(KRTCreatorContext &ctx)
    : KRTAbsItemCreator(report_template::STR_RT_ELEMENT_IMAGE_BLOCK, ctx) {}

KRTImageGroupCreator::KRTImageGroupCreator(KRTCreatorContext &ctx)
    : KRTAbsItemCreator(report_template::STR_RT_ELEMENT_IMAGEGROUP_BLOCK, ctx) {}

// Тип параметром: один класс регистрируется ДВАЖДЫ — под TABLE и TITLE_TABLE.
KRTTableItemCreator::KRTTableItemCreator(const std::string &type,
                                         KRTCreatorContext &ctx)
    : KRTAbsItemCreator(type, ctx) {}

KRTSubDataItemCreator::KRTSubDataItemCreator(KRTCreatorContext &ctx)
    : KRTAbsItemCreator(report_template::STR_RT_ELEMENT_SUB_DATA_BLOCK, ctx) {}

// --- KRTCreatorContext ------------------------------------------------------

KRTCreatorContext::KRTCreatorContext(KReportTemplateDataNew *pData)
    : m_pData(pData)
{
    // Реф. ctor @0x547ff0: vptr, пустая map, m_pData, затем InitCreator().
    InitCreator();
}

KRTCreatorContext::~KRTCreatorContext()
{
    // Реф. dtor @0x547300/@0x5473d8: обход map, delete каждого творца, _M_erase.
    // ВАЖНО: один и тот же КЛАСС зарегистрирован дважды, но это ДВА РАЗНЫХ
    // объекта (два operator new) — двойного удаления нет.
    for (auto &kv : m_creators)
        delete kv.second;
    m_creators.clear();
}

void KRTCreatorContext::InitCreator()
{
    using namespace report_template;
    // Порядок и состав — дизасм InitCreator @0x547918: ровно 6 operator new
    // и 6 вставок; KRTTableItemCreator::ctor встречается ДВАЖДЫ.
    AddNewCreatorType(STR_RT_ELEMENT_TEXT_BLOCK,
                      new KRTTextItemCreator(*this));
    AddNewCreatorType(STR_RT_ELEMENT_IMAGE_BLOCK,
                      new KRTImageItemCreator(*this));
    AddNewCreatorType(STR_RT_ELEMENT_IMAGEGROUP_BLOCK,
                      new KRTImageGroupCreator(*this));
    AddNewCreatorType(STR_RT_ELEMENT_TABLE_BLOCK,
                      new KRTTableItemCreator(STR_RT_ELEMENT_TABLE_BLOCK, *this));
    AddNewCreatorType(STR_RT_ELEMENT_TITLE_TABLE_BLOCK,
                      new KRTTableItemCreator(STR_RT_ELEMENT_TITLE_TABLE_BLOCK, *this));
    AddNewCreatorType(STR_RT_ELEMENT_SUB_DATA_BLOCK,
                      new KRTSubDataItemCreator(*this));
}

bool KRTCreatorContext::AddNewCreatorType(const std::string &type,
                                          KRTAbsItemCreator *pCreator)
{
    // Реф. @0x548050: insert только при отсутствии ключа. Если ключ занят —
    // объект НЕ подменяется; чтобы не течь, лишний экземпляр удаляем.
    if (!pCreator)
        return false;
    auto it = m_creators.find(type);
    if (it != m_creators.end()) {
        delete pCreator;
        return false;
    }
    m_creators[type] = pCreator;
    return true;
}

KRTAbsItemCreator *KRTCreatorContext::FindCreator(const std::string &type) const
{
    // Реф. CreateBlock: первый поиск по типу; при промахе — ВТОРОЙ поиск
    // по STR_RT_ELEMENT_TEXT_BLOCK; при повторном промахе false.
    auto it = m_creators.find(type);
    if (it != m_creators.end())
        return it->second;
    it = m_creators.find(report_template::STR_RT_ELEMENT_TEXT_BLOCK);
    return it != m_creators.end() ? it->second : nullptr;
}

KRTAbsItemCreator *KRTCreatorContext::FindCreatorForUpdate(const std::string &type) const
{
    // Реф. UpdateBlock(…QTextFrame*) @0x546788: фолбэк ДРУГОЙ — TABLE_BLOCK.
    auto it = m_creators.find(type);
    if (it != m_creators.end())
        return it->second;
    it = m_creators.find(report_template::STR_RT_ELEMENT_TABLE_BLOCK);
    return it != m_creators.end() ? it->second : nullptr;
}

bool KRTCreatorContext::CreateBlock(const std::string &type,
                                    KReportTemplateItem *pItem,
                                    QTextTableCell &cell)
{
    KRTAbsItemCreator *p = FindCreator(type);
    return p ? p->CreateBlock(pItem, cell) : false;
}

bool KRTCreatorContext::CreateBlock(const std::string &type,
                                    KReportTemplateItem *pItem,
                                    QTextFrame *pFrame)
{
    KRTAbsItemCreator *p = FindCreator(type);
    return p ? p->CreateBlock(pItem, pFrame) : false;
}

bool KRTCreatorContext::UpdateBlock(const std::string &type,
                                    KReportTemplateItem *pItem,
                                    QTextFrame *pFrame)
{
    // Реф. @0x546788: очистка фрейма (Qt) + вызов творца. Очистка появится
    // вместе с рендером; здесь — только диспетчеризация с TABLE-фолбэком.
    KRTAbsItemCreator *p = FindCreatorForUpdate(type);
    return p ? p->UpdateBlock(pItem, pFrame) : false;
}
