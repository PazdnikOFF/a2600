#pragma once

#include <map>
#include <memory>
#include <string>

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

    KRTAbsDataSource    *DataSource() const { return m_pDataSource; }
    KReportDisplayParam *DisplayParam() const { return m_pDisplayParam; }

private:
    KRTAbsDataSource    *m_pDataSource = nullptr;     // +0x08 (не владеет)
    KReportDisplayParam *m_pDisplayParam = nullptr;   // +0x10 (не владеет)
    std::map<std::string, std::unique_ptr<KRTTeAbsItemCreator>> m_mapCreators;
    int m_defaultFontSize = 12;
};
