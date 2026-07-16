#pragma once

#include "report/KReportTemplateData.h"
#include "report/KTextBlock.h"

#include <QString>
#include <QSize>
#include <QTextFormat>   // QTextLength
#include <QVector>

#include <string>
#include <vector>

// Модель табличного блока отчёта (реф. KTableBlock, X-2600). Третий сиблинг
// KTextBlock/KImageBlock — тоже обёртка над KReportTemplateItem* + KReportTemplateDataNew*,
// НО, в отличие от них, ПОЛИМОРФНАЯ: у реф. свой vtable (virtual dtor + Size +
// GetTemplateItemForCell), хотя базового класса и виджет-зависимостей нет (sizeof 0x78,
// off-device). Пиксельный рендер таблицы — у потребителей (KRTTableItemCreator,
// KTemplateEditDocument::InsertTable), их реимплементировать не надо.
//
// Отличия от текст/картинка-сиблингов: НЕТ m_mapTextParam; добавлен вычисляемый
// QSize m_size (сетка: width=строки, height=столбцы) — считает CalcTableSize() в ctor.
// ElementId/ShowTitle/TableType дереференсят m_pTemplateItem БЕЗ null-guard (как реф.).
class KTableBlock
{
public:
    KTableBlock(KReportTemplateItem *item, KReportTemplateDataNew *dataNew);
    virtual ~KTableBlock() = default;

    // Размер сетки таблицы: width() = число строк, height() = число столбцов.
    // (реф. virtual, слот vtable +0x20; просто отдаёт кэш m_size.)
    virtual QSize Size() const;

    // Элемент под-дерева для ячейки (row, col), row-major: index = row*cols + col,
    // cols = Size().height(). out ← адрес объекта в m_lstSubItems; true при попадании.
    // row<0/col<0 → лог + false (out не трогает); index вне списка → false.
    // (реф. virtual, слот vtable +0x28.)
    virtual bool GetTemplateItemForCell(int row, int col, KReportTemplateItem *&out) const;

    QString ElementId() const;              // m_pTemplateItem->m_strID (БЕЗ null-guard)
    bool    ShowTitle() const;              // m_pTemplateItem->m_strShowTitle == "1" (поле узла!)
    KTextBlock TitleTextBlock() const;      // KTextBlock(item, dataNew, hideKey=true)
    QString TableType() const;              // m_strType с ремапом 7 типов → RT_TITLE_TABLE_BLOCK

    float BorderWidth() const;              // item-config "BorderWidth" → toFloat, 0.0f если нет
    bool  ShowBorder() const;               // BorderWidth() != 0.0
    std::vector<float> Margin() const;      // item-config "MarginWidth" → split "," → toFloat*0.62
    QString BorderColor() const;            // item-config "BorderColor" → строка, дефолт "black"
    QVector<QTextLength> ColWidthContraints() const;   // item-config "ColumnRatio" → % ширины

    KReportTemplateItem       *GetTemplateItem() const { return m_pTemplateItem; }
    KReportTemplateDataNew    *GetTemplateData() const { return m_pDataNew; }
    KReportTemplateItemConfig  GetItemConfig() const { return m_itemConfig; }

private:
    void InitItemConfig();          // m_itemConfig = dataNew->m_mapItemConfigs[item->m_strID]
    void CalcTableSize();           // считает m_size из Column/RefColumnID + числа детей
    std::string attr(const std::string &key) const;   // item-config-атрибут ("" если нет)

    KReportTemplateItem       *m_pTemplateItem = nullptr;   // +0x08
    KReportTemplateDataNew    *m_pDataNew = nullptr;        // +0x10
    KReportTemplateItemConfig  m_itemConfig;                // +0x18
    QSize                      m_size;                      // +0x70 (width=строки, height=столбцы)
};
