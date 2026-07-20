#pragma once

#include <list>
#include <string>

struct KReportTemplateItem;
class KRTCreatorContext;

// Qt-типы только объявлены: базовые реализации их НЕ трогают, поэтому этот
// заголовок и его .cpp не тянут Qt вообще (ссылка на неполный тип в объявлении
// метода допустима). Рендер появится в следующей итерации вместе с творцами.
class QTextTableCell;
class QTextFrame;
class QTextCursor;
class QTextTable;
class KTextBlock;
class KImageBlock;
class KTableBlock;

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

    // Реф. CreateBlock(item*, frame*) @0x53b8b0 — ОБЁРТКА: строит KTextBlock, вкладывает
    // блок в отдельный QTextFrame (frame-формат несёт ElementId в property UserProperty+1),
    // зовёт рисующую перегрузку, затем m_context.HideInvalidBlock(inner) и (frame).
    bool CreateBlock(KReportTemplateItem *pItem, QTextFrame *pFrame) override;

    // Реф. CreateBlock(item*, QTextTableCell&) @0x53b798 — текст ПРЯМО в ячейку таблицы
    // (без вложенного фрейма): cell.lastCursorPosition() → рисующая перегрузка.
    bool CreateBlock(KReportTemplateItem *pItem, QTextTableCell &cell) override;

private:
    // Реф. CreateBlock(KTextBlock const&, QTextCursor&) @0x53b3b0 — РИСОВАНИЕ:
    // QTextCharFormat (Bold→FontWeight 75, Italic, FontPointSize=Size×KScreenMng::
    // GetRatioTo1K, ForegroundBrush из FontColor), QTextBlockFormat (Alignment),
    // insertBlock + insertText(FullText). Имя наше (реф. — перегрузка CreateBlock).
    bool renderBlock(const KTextBlock &block, QTextCursor &cur);
};

class KRTImageItemCreator : public KRTAbsItemCreator
{
public:
    explicit KRTImageItemCreator(KRTCreatorContext &ctx);

    // Реф. CreateBlock(item*, frame*) @0x536240 — обёртка: KImageBlock + вложенный QTextFrame
    // с ElementId, рисование, HideInvalidBlock. (В прошивке KRTImageItemCreator наследует
    // KRTTableItemCreator и строит внешний фрейм через KTableBlock; у нас иерархия уплощена,
    // фрейм-формат берём как у текста + ElementId — на визуал min-рендера не влияет.)
    bool CreateBlock(KReportTemplateItem *pItem, QTextFrame *pFrame) override;

private:
    // Реф. CreateBlock(item*, QTextCursor&) @0x535c40 — РИСОВАНИЕ: если Url невалиден (файла
    // нет) — НИЧЕГО не вставляет (пустой блок спрячет HideInvalidBlock); иначе QTextImageFormat
    // (ImageName=url, ImageWidth/Height если >0) + QTextBlockFormat (Alignment + keep-маркер
    // UserProperty+2), insertBlock + insertImage. Масштаб GetRatioTo1K НЕ применяется (размеры
    // уже в device-px). Имя наше.
    bool renderImage(const KImageBlock &img, QTextCursor &cur);
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

    // Реф. CreateBlock(item*, QTextFrame*) @0x53a400 — обёртка: KTableBlock → QTextTable
    // (rows×cols из Size, +1 строка при ShowTitle) → заполнение ячеек рекурсией в творцов.
    // МИНИМУМ: single-column-фрейм-ветка (CreateFrame) опущена (всегда таблица, помечено).
    bool CreateBlock(KReportTemplateItem *pItem, QTextFrame *pFrame) override;

private:
    // Реф. CreateTable @0x53a6f0: QTextTableFormat (border/borderColor/colWidth) →
    // cursor.insertTable(rows,cols); при ShowTitle — merge строки 0 + insertTitle.
    QTextTable *createTable(QTextCursor &cur, const KTableBlock &blk);
    // Реф. CreateChild(list, table, hasTitle) @0x539d08: проход по дочерним item-ам,
    // позиция idx/cols × idx%cols (+1 строка при hasTitle), CreateBlock(type,item,cell).
    void createChild(std::list<KReportTemplateItem> &items, QTextTable *pTable,
                     bool hasTitle);
    // Реф. InsertTitle @0x539448: заголовок таблицы в курсор (min: insertText(FullText)).
    void insertTitle(QTextCursor &cur, const KTextBlock &title);
};

class KRTSubDataItemCreator : public KRTAbsItemCreator
{
public:
    explicit KRTSubDataItemCreator(KRTCreatorContext &ctx);
};
