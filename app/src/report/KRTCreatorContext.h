#pragma once

#include <map>
#include <string>

struct KReportTemplateDataNew;
struct KReportTemplateItem;
class KRTAbsItemCreator;
class QTextTableCell;
class QTextFrame;

// Строковые типы блоков отчёта (реф. константы report_template::STR_RT_ELEMENT_*).
// Значения подтверждены РЕАЛЬНЫМИ шаблонами поставки (атрибут Type= в <Content>):
//   RT_TEXT_BLOCK 237 вхождений, RT_TABLE_BLOCK 93, RT_TITLE_TABLE_BLOCK 89,
//   RT_IMAGE_BLOCK 62. RT_IMAGEGROUP_BLOCK и RT_SUB_DATA_BLOCK зарегистрированы
//   в коде, но в поставляемых шаблонах НЕ ВСТРЕЧАЮТСЯ НИ РАЗУ.
// Всего констант STR_RT_ELEMENT_* в бинарнике 23 — в карту творцов попадают 6,
// остальные 17 уходят в фолбэк на TEXT_BLOCK.
namespace report_template {
extern const std::string STR_RT_ELEMENT_TEXT_BLOCK;         // "RT_TEXT_BLOCK"
extern const std::string STR_RT_ELEMENT_IMAGE_BLOCK;        // "RT_IMAGE_BLOCK"
extern const std::string STR_RT_ELEMENT_IMAGEGROUP_BLOCK;   // "RT_IMAGEGROUP_BLOCK"
extern const std::string STR_RT_ELEMENT_TABLE_BLOCK;        // "RT_TABLE_BLOCK"
extern const std::string STR_RT_ELEMENT_TITLE_TABLE_BLOCK;  // "RT_TITLE_TABLE_BLOCK"
extern const std::string STR_RT_ELEMENT_SUB_DATA_BLOCK;     // "RT_SUB_DATA_BLOCK"
} // namespace report_template

// Контекст создания блоков отчёта (реф. KRTCreatorContext, sizeof 0x40).
// Раскладка реф.: +0x00 vptr (полиморфен ТОЛЬКО из-за виртуального dtor — _ZTV
// всего 0x20, _ZTI = __class_type_info без баз), +0x08 std::map<std::string,
// KRTAbsItemCreator*> m_creators, +0x38 KReportTemplateDataNew* m_pData.
//
// Владелец — KDocumentGenerator (создаёт `new` в ctor). ⚠️ В РЕФ. dtor
// KTemplateEditDocument этот объект НЕ УДАЛЯЛ (утечка 0x40 байт); у самого
// KRTCreatorContext dtor корректный — обходит map и delete'ит творцов.
//
// СТАТУС: реализованы реестр и диспетчеризация (чистый STL, Qt не участвует).
// Рендер (CreateBlock/UpdateBlock с QTextCursor/QTextTable, GetFontSize с
// QGuiApplication::primaryScreen) — следующая итерация.
class KRTCreatorContext
{
public:
    explicit KRTCreatorContext(KReportTemplateDataNew *pData);
    virtual ~KRTCreatorContext();

    KRTCreatorContext(const KRTCreatorContext &) = delete;
    KRTCreatorContext &operator=(const KRTCreatorContext &) = delete;

    // Реф. InitCreator @0x547918: регистрирует 6 творцов на 5 классов.
    // Доказано дизасмом: 6 вызовов operator new + 6 вставок в map;
    // KRTTableItemCreator::ctor вызывается ДВАЖДЫ, остальные — по разу.
    void InitCreator();

    // Реф. AddNewCreatorType @0x548050: вставка, ТОЛЬКО если ключа ещё нет
    // (повторная регистрация того же типа игнорируется, объект НЕ подменяется).
    // Возврат — наш (реф. void): true, если вставка состоялась.
    bool AddNewCreatorType(const std::string &type, KRTAbsItemCreator *pCreator);

    // Поиск творца с фолбэком. В реф. это ИНЛАЙН внутри CreateBlock/UpdateBlock;
    // выделено в метод (наше) ради проверяемости.
    //   CreateBlock            → промах даёт фолбэк на STR_RT_ELEMENT_TEXT_BLOCK
    //   UpdateBlock(QTextFrame*) → промах даёт фолбэк на STR_RT_ELEMENT_TABLE_BLOCK
    // Если и фолбэк не найден — nullptr (реф. возвращает false).
    KRTAbsItemCreator *FindCreator(const std::string &type) const;
    KRTAbsItemCreator *FindCreatorForUpdate(const std::string &type) const;

    // Реф. CreateBlock @0x546558 / @0x5466e8, UpdateBlock @0x5465f8 / @0x546788.
    bool CreateBlock(const std::string &type, KReportTemplateItem *pItem,
                     QTextTableCell &cell);
    bool CreateBlock(const std::string &type, KReportTemplateItem *pItem,
                     QTextFrame *pFrame);
    bool UpdateBlock(const std::string &type, KReportTemplateItem *pItem,
                     QTextFrame *pFrame);

    // Для проверок: сколько типов зарегистрировано (реф. поля не имеет).
    std::size_t CreatorCount() const { return m_creators.size(); }

private:
    std::map<std::string, KRTAbsItemCreator *> m_creators;  // +0x08
    KReportTemplateDataNew *m_pData = nullptr;              // +0x38
};
