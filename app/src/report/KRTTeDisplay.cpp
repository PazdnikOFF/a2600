#include "report/KRTTeDisplay.h"

#include "report/KRTTeCreatorContext.h"
#include "report/KReportTemplateCommonDef.h"

#include <QAbstractTextDocumentLayout>
#include <QColor>
#include <QPrinter>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextFrame>
#include <QTextTable>

// Ключ цвета фона документа в конфигах шаблона (реф. литерал @0x874b58).
static const char *kBgColor = "BgColor";

KRTTeDisplay::KRTTeDisplay(KRTAbsDataSource *pDataSource)
    : KRTAbsDisplay(pDataSource)
{
    // Реф. ctor @0x5121e8: база → зануление +0xE0/+0xE8 → InitMembers().
    InitMembers();
}

KRTTeDisplay::~KRTTeDisplay()
{
    // Реф. dtor @0x5120c8: сперва контекст (+0xE8), затем документ (+0xE0), затем база.
    m_pContext.reset();
    delete m_pDocument;
    m_pDocument = nullptr;
}

void KRTTeDisplay::InitMembers()
{
    // Реф. @0x512198: new KRTTeCreatorContext(m_pDataSource, &m_displayParam).
    m_pContext = std::make_unique<KRTTeCreatorContext>(m_pDataSource, &m_displayParam);
}

bool KRTTeDisplay::Reset()
{
    // Реф. @0x512150: параметр → контекст → удалить документ. Возврат безусловно true.
    m_displayParam.Reset();
    if (m_pContext)
        m_pContext->Reset();
    delete m_pDocument;
    m_pDocument = nullptr;
    return true;
}

void KRTTeDisplay::SetRefValidItems(const std::set<std::string> &items)
{
    m_displayParam.SetRefValidItems(items);   // реф. @0x512238 — форвард на +0x10
}

bool KRTTeDisplay::Display(const KReportTemplateDataNew &data)
{
    return DisplayImpl(data, nullptr);
}

bool KRTTeDisplay::Display(const KReportTemplateDataNew &data, QPrinter *printer)
{
    // Реф. @0x512ae8: проверка идёт ДО пролога — nullptr даёт немедленный false.
    if (!printer)
        return false;
    return DisplayImpl(data, printer);
}

bool KRTTeDisplay::DisplayImpl(const KReportTemplateDataNew &data, QPrinter *printer)
{
    // Общий каркас обеих перегрузок (реф. @0x512240 / @0x512ae8).
    delete m_pDocument;
    m_pDocument = new QTextDocument(nullptr);
    m_pDocument->setUndoRedoEnabled(false);

    if (printer) {
        // Только печатная перегрузка: paint device и размер страницы от принтера.
        m_pDocument->documentLayout()->setPaintDevice(printer);
        m_pDocument->setPageSize(printer->pageRect().size());
    }

    QTextCursor cursor(m_pDocument);
    cursor.beginEditBlock();

    // Фон корневого фрейма из конфига шаблона по ключу "BgColor"; margin 30.
    QTextFrameFormat rootFmt = m_pDocument->rootFrame()->frameFormat();
    rootFmt.setMargin(30);
    auto bg = data.m_mapConfigs.find(kBgColor);
    if (bg != data.m_mapConfigs.end()) {
        QColor c;
        c.setNamedColor(QString::fromStdString(bg->second));
        if (c.isValid())
            rootFmt.setBackground(c);
    }
    m_pDocument->rootFrame()->setFrameFormat(rootFmt);

    if (m_pContext) {
        QFont f = m_pDocument->defaultFont();
        f.setPointSize(m_pContext->GetFontSize(nullptr));
        m_pDocument->setDefaultFont(f);
    }

    m_displayParam.UpdateTemplateDisplayParam(data.m_mapConfigs, data.m_mapItemConfigs);

    // Реф. читает счётчик элементов по +0x40 — это size() списка m_lstItems (+0x30).
    const int itemCount = static_cast<int>(data.m_lstItems.size());
    if (itemCount <= 0) {
        cursor.endEditBlock();
        return false;
    }

    QTextTableFormat tblFmt;
    QVector<QTextLength> widths;
    widths << QTextLength(QTextLength::PercentageLength, 100);
    tblFmt.setColumnWidthConstraints(widths);
    QTextTable *table = cursor.insertTable(itemCount, 1, tblFmt);
    if (!table) {
        // Реф. здесь падает по assert — в порте выходим аккуратно.
        cursor.endEditBlock();
        return false;
    }

    int filled = 0;
    for (const KReportTemplateItem &item : data.m_lstItems) {
        // Реф. подбирает конфиг элемента и передаёт его карту атрибутов творцу.
        std::map<std::string, std::string> cfgMap;
        auto cfg = data.m_mapItemConfigs.find(item.m_strID);
        if (cfg != data.m_mapItemConfigs.end())
            cfgMap = cfg->second.m_mapAttrs;

        QTextTableCell cell = table->cellAt(filled, 0);
        int rc = 0;
        if (m_pContext)
            rc = m_pContext->CreateItem(item, cfgMap, cell);
        if (rc != 0)
            ++filled;

        if (printer && m_pContext) {
            // Только печать: разрывные линии между блоками.
            KSplitLineInfo info;
            report_template::GetSplitLineInfo(item.m_strID, data.m_mapItemConfigs, info);
            if (info.m_nSplitLineWidth > 0)
                m_pContext->InsertSplitLine(info, table);
        }
    }

    // Реф.: лишние (незаполненные) строки удаляются.
    if (filled < itemCount)
        table->removeRows(filled, itemCount - filled);

    cursor.endEditBlock();
    return true;
}
