#pragma once

#include <string>

struct KReportTemplateItem;
class KRTCreatorContext;

// Qt-типы только объявлены: базовые реализации их НЕ трогают, поэтому этот
// заголовок и его .cpp не тянут Qt вообще (ссылка на неполный тип в объявлении
// метода допустима). Рендер появится в следующей итерации вместе с творцами.
class QTextTableCell;
class QTextFrame;

// База иерархии «творцов» блоков отчёта (реф. KRTAbsItemCreator, sizeof 0x30).
// Раскладка реф.: +0x00 vptr, +0x08 std::string m_strType (SSO-буфер @0x18),
// +0x28 KRTCreatorContext& m_context.
//
// vtable реф. (5 слотов): +0x00 ~D1, +0x08 ~D0,
//   +0x10 CreateBlock(KReportTemplateItem*, QTextTableCell&)
//   +0x18 CreateBlock(KReportTemplateItem*, QTextFrame*)
//   +0x20 UpdateBlock(KReportTemplateItem*, QTextFrame*)
// Базовые реализации всех трёх в реф. — `printf(<m_strType>); return true;`,
// т.е. заглушки. Здесь они возвращают true без печати.
//
// ПРИМЕЧАНИЕ ПО ФАЙЛАМ: в реф. каждый конкретный творец — отдельный TU
// (KRTTextItemCreator @0x53b330, KRTImageItemCreator @0x535bd0,
// KRTImageGroupCreator @0x5343a0, KRTTableItemCreator @0x539408,
// KRTSubDataItemCreator @0x5365d0). У нас они собраны в один файл ради
// компактности — ИМЕНА КЛАССОВ при этом 1:1 с прошивкой.
class KRTAbsItemCreator
{
public:
    KRTAbsItemCreator(const std::string &type, KRTCreatorContext &ctx)
        : m_strType(type), m_context(ctx) {}
    virtual ~KRTAbsItemCreator() = default;

    // Реф. GetType() возвращает КОПИЮ m_strType.
    std::string GetType() const { return m_strType; }

    virtual bool CreateBlock(KReportTemplateItem *pItem, QTextTableCell &cell);
    virtual bool CreateBlock(KReportTemplateItem *pItem, QTextFrame *pFrame);
    virtual bool UpdateBlock(KReportTemplateItem *pItem, QTextFrame *pFrame);

protected:
    std::string        m_strType;   // +0x08
    KRTCreatorContext &m_context;   // +0x28
};

// --- Конкретные творцы (реф.-имена 1:1) ------------------------------------
// Рендер (QTextCursor/QTextTable/insertImage) — СЛЕДУЮЩАЯ итерация; сейчас это
// корректные экземпляры с правильным типом, нужные для проверки диспетчера.

class KRTTextItemCreator : public KRTAbsItemCreator
{
public:
    explicit KRTTextItemCreator(KRTCreatorContext &ctx);
};

class KRTImageItemCreator : public KRTAbsItemCreator
{
public:
    explicit KRTImageItemCreator(KRTCreatorContext &ctx);
};

class KRTImageGroupCreator : public KRTAbsItemCreator
{
public:
    explicit KRTImageGroupCreator(KRTCreatorContext &ctx);
};

// ВНИМАНИЕ: в реф. ОДИН класс обслуживает ДВА типа — регистрируется двумя
// экземплярами (RT_TABLE_BLOCK и RT_TITLE_TABLE_BLOCK), поэтому тип передаётся
// параметром, а не зашит в классе. Доказано дизасмом InitCreator @0x547918:
// KRTTableItemCreator::ctor вызывается ДВАЖДЫ, остальные — по одному разу.
class KRTTableItemCreator : public KRTAbsItemCreator
{
public:
    KRTTableItemCreator(const std::string &type, KRTCreatorContext &ctx);
};

class KRTSubDataItemCreator : public KRTAbsItemCreator
{
public:
    explicit KRTSubDataItemCreator(KRTCreatorContext &ctx);
};
