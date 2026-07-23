#pragma once

#include <map>
#include <memory>
#include <string>

#include <QColor>

#include "report/KReportTemplateData.h"

class KRTAbsDataSource;
class KReportDisplayParam;
class KRTTeAbsItemCreator;
class QTextTable;
class QTextTableCell;

// Контекст Te-движка (реф. KRTTeCreatorContext, ctor @0x50d3e8, sizeof 0x98).
// Как и Simple-контекст, получает источник данных и УКАЗАТЕЛЬ на KReportDisplayParam,
// встроенный по значению в KRTTeDisplay (+0x10) — им НЕ владеет.
//
// ⚠️ ГРАНИЦА ЭТОЙ ИТЕРАЦИИ. У реф. класса ~20 методов (FillSplitCell @0x50b5d8,
// IsPedResultItem, IsTemplatePreviewState, GetFontColor, CheckTitleMerge,
// IsGetFontChineseShow, GetFontSize ×2, IsMutiDeptTemplate, InsertSplitLine @0x50c1c0,
// InitCreator @0x50c9d0, CleanTitleMerge, UpdateTitleMerge, HandleLrRepeat @0x50e208,
// HandleRepeat @0x50fb08, CreateItem @0x5102f8). Здесь портирован КАРКАС: владение,
// Reset и диспетчер CreateItem через инъектируемый реестр творцов. Тела остальных —
// следующий заход; они честно помечены как НЕ ПОРТИРОВАННЫЕ.
class KRTTeCreatorContext
{
public:
    KRTTeCreatorContext(KRTAbsDataSource *pDataSource, KReportDisplayParam *pDisplayParam);
    virtual ~KRTTeCreatorContext();

    KRTTeCreatorContext(const KRTTeCreatorContext &) = delete;
    KRTTeCreatorContext &operator=(const KRTTeCreatorContext &) = delete;

    void Reset();   // реф. @0x50d5b8

    // Реф. @0x50c9d0 — СЕМЬ пар «ключ → творец» (у Simple-движка их ДЕВЯТЬ).
    // Совпадают: RT_TEXT_BLOCK, RT_TEXTGROUP_BLOCK, RT_IMAGE_BLOCK, RT_IMAGEGROUP_BLOCK,
    // RT_TABLE_BLOCK, RT_TITLE_TABLE_BLOCK, RT_SUB_DATA_BLOCK.
    // ⚠️ У Te ОТСУТСТВУЮТ `RT_ROW_TABLE_BLOCK` и `RT_OB_Z_SCORE_BLOCK`: Simple-движок
    // считает их валидность, но Te их НЕ РЕНДЕРИТ вовсе. Ctor каждого творца принимает
    // ТОЛЬКО контекст — строкового подтипа (как у «взрослого» KRTTableItemCreator) нет,
    // TABLE и TITLE_TABLE получают одинаковый вызов.
    void InitCreator();

    // Реф. @0x5102f8 — диспетчер в конкретного Te-творца по типу блока и рендер в ячейку.
    // Реестр (реф. InitCreator @0x50c9d0) в этой итерации НЕ реверсирован ⇒ инъекция.
    int CreateItem(const KReportTemplateItem &item,
                   const std::map<std::string, std::string> &cfgMap,
                   QTextTableCell &cell);
    void RegisterCreator(const std::string &type, std::unique_ptr<KRTTeAbsItemCreator> c);

    // Реф. @0x50bed8 — размер шрифта по умолчанию для документа (аргумент — элемент или
    // nullptr). Тело НЕ реверсировано; в порте — настраиваемое значение.
    int GetFontSize(const KReportTemplateItem *item) const;
    void SetDefaultFontSize(int px) { m_defaultFontSize = px; }

    // Реф. @0x50c1c0 — вставка «разрывной» строки при печати. Тело НЕ реверсировано.
    void InsertSplitLine(const KSplitLineInfo &info, QTextTable *table);

    // Реф. @0x50b918 / @0x50bba0 — тела НЕ реверсированы; в порте настраиваемые сеймы.
    QColor GetFontColor(const KReportTemplateItem *item) const;
    void SetDefaultFontColor(const QColor &c) { m_defaultFontColor = c; }
    bool IsGetFontChineseShow(const KReportTemplateItem *item) const;

    // Реф. поле +0x48 (int): режим, сравнивается с 1 в KRTTeTextItemCreator::CreateItem
    // и с 2 в его GetItemTitle. НАЗНАЧЕНИЕ НЕ ВОССТАНОВЛЕНО — оставлено настраиваемым.
    int  Mode() const { return m_nMode; }
    void SetMode(int m) { m_nMode = m; }

    KRTAbsDataSource    *DataSource() const { return m_pDataSource; }
    KReportDisplayParam *DisplayParam() const { return m_pDisplayParam; }

private:
    KRTAbsDataSource    *m_pDataSource = nullptr;     // +0x08 (не владеет)
    KReportDisplayParam *m_pDisplayParam = nullptr;   // +0x10 (не владеет)
    std::map<std::string, std::unique_ptr<KRTTeAbsItemCreator>> m_mapCreators;
    int    m_defaultFontSize = 12;
    QColor m_defaultFontColor = Qt::black;
    int    m_nMode = 0;                      // +0x48
};
