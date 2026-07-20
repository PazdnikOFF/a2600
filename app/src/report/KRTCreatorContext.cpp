#include "KRTCreatorContext.h"

#include "KRTAbsItemCreator.h"
#include "KTextBlock.h"
#include "ui/KScreenMng.h"

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextFrame>
#include <QTextFrameFormat>

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

void KRTCreatorContext::HideInvalidBlock(QTextFrame *pFrame)
{
    // Реф. @0x546168: скрыть пустые/невалидные блоки фрейма, кроме помеченных keep-флагом
    // (property UserProperty+2). Скрытие — QTextBlock::setVisible(false) (drop из layout,
    // без удаления/правки курсора). void.
    if (!pFrame)
        return;
    const int keepProp = QTextFormat::UserProperty + 2;   // 0x100002
    for (QTextFrame::iterator it = pFrame->begin(); it != pFrame->end(); ++it) {
        QTextBlock b = it.currentBlock();
        if (b.blockFormat().property(keepProp).toBool())
            continue;                       // явно помечен «оставить»
        if (!b.isValid())
            b.setVisible(false);
        if (b.text().isEmpty())
            b.setVisible(false);
    }
}

// --- KRTTextItemCreator: рендер текстового блока (реф. TU @0x53b330) ----------

bool KRTTextItemCreator::CreateBlock(KReportTemplateItem *pItem, QTextFrame *pFrame)
{
    // Реф. @0x53b8b0: обёртка. Вкладывает блок в отдельный QTextFrame, на frame-формат
    // вешает ElementId (UserProperty+1) — по нему потом идёт выделение блока.
    if (!pItem || !pFrame)
        return false;

    KTextBlock block(pItem, m_context.GetData(), /*hideKey=*/false);

    QTextCursor cur = pFrame->lastCursorPosition();
    cur.beginEditBlock();
    QTextFrameFormat frameFmt = pFrame->frameFormat();
    frameFmt.setProperty(QTextFormat::UserProperty + 1,          // 0x100001
                         QVariant(block.ElementId()));
    QTextFrame *inner = cur.insertFrame(frameFmt);
    cur.endEditBlock();

    QTextCursor innerCur = inner->lastCursorPosition();
    const bool ok = renderBlock(block, innerCur);

    // Спрятать пустые плейсхолдер-блоки внутреннего и внешнего фреймов.
    m_context.HideInvalidBlock(inner);
    m_context.HideInvalidBlock(pFrame);
    return ok;
}

bool KRTTextItemCreator::renderBlock(const KTextBlock &block, QTextCursor &cur)
{
    // Реф. @0x53b3b0. Высокоуровневые Qt-сеттеры ставят РОВНО те property-id, что сверены
    // дизасмом: setFontWeight→FontWeight(0x2003)=75, setFontItalic→FontItalic(0x2004),
    // setFontPointSize→FontPointSize(0x2001), setForeground→ForegroundBrush(0x821),
    // setAlignment→BlockAlignment(0x1010).
    QTextCharFormat charFmt = cur.charFormat();
    if (block.Bold())
        charFmt.setFontWeight(QFont::Bold);        // 75
    if (block.Italic())
        charFmt.setFontItalic(true);

    int sz = -1;
    if (block.FontSize(sz)) {   // возвращает pt; 0 → размер не задан, пропуск
        // Масштаб pt под фактический экран (реф. size×KScreenMng::GetRatioTo1K).
        const double pt = static_cast<double>(sz) * KScreenMng::GetInstance()->GetRatioTo1K();
        charFmt.setFontPointSize(pt);
    }

    QColor col;
    if (block.FontColor(col) && col.isValid())     // FontColor всегда true → гейт по isValid
        charFmt.setForeground(QBrush(col, Qt::SolidPattern));

    QTextBlockFormat blockFmt = cur.blockFormat();
    QFlags<Qt::AlignmentFlag> al = Qt::AlignLeft;
    if (block.Alignment(al))                        // только если AlignH задан явно
        blockFmt.setAlignment(al);

    // ОТСТУПЛЕНИЕ ОТ РЕФ. (помечено): реф. дополнительно кладёт САМ KTextBlock в property
    // UserProperty(0x100000) как QVariant-метатип "KTextBlock" — машинерия round-trip для
    // редактора (retrieve блока из документа). На ВИЗУАЛЬНЫЙ рендер не влияет; требует
    // Q_DECLARE_METATYPE(KTextBlock) — добавим при реализации Change*Selected/редактора.

    cur.beginEditBlock();
    cur.insertBlock(blockFmt, charFmt);
    cur.insertText(block.FullText());
    cur.endEditBlock();
    return true;
}
