#include "KRTCreatorContext.h"

#include "KRTAbsItemCreator.h"
#include "KTextBlock.h"
#include "KImageBlock.h"
#include "KTableBlock.h"
#include "KMeaStringUtil.h"
#include "ui/KScreenMng.h"

#include <QtGlobal>
#include <QGuiApplication>
#include <QScreen>
#include <QString>
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextFrame>
#include <QTextFrameFormat>
#include <QTextImageFormat>
#include <QTextTable>
#include <QTextTableCell>
#include <QTextTableFormat>

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

namespace {
// База шрифта отчёта (реф. QFont("Source Han Sans CN", 16, Normal, false), обе перегрузки).
QFont baseReportFont()
{
    return QFont(QStringLiteral("Source Han Sans CN"), 16, QFont::Normal, false);
}
// DPI-скейл (реф. общий хвост @0x547574): setPointSize(qRound(pt×dpi/150)); без экрана dpi=150.
void applyDpiScale(QFont &f)
{
    double dpi = 150.0;
    if (QScreen *s = QGuiApplication::primaryScreen())
        dpi = s->physicalDotsPerInch();
    f.setPointSize(qRound(f.pointSize() * dpi / 150.0));
}
} // namespace

QFont KRTCreatorContext::GetFontSize(const std::string &styleName) const
{
    // Реф. @0x547400: именованный стиль → Size/Bold/Italic, затем DPI-скейл (всегда).
    QFont f = baseReportFont();
    if (m_pData) {
        auto it = m_pData->m_mapItemConfigs.find(styleName);
        if (it != m_pData->m_mapItemConfigs.end()) {
            const auto &attrs = it->second.m_mapAttrs;
            auto s = attrs.find("Size");
            if (s != attrs.end()) {
                KMeaStringUtil u;
                f.setPointSize(u.ConvertStringToInt(s->second));
            }
            auto b = attrs.find("Bold");
            if (b != attrs.end() && b->second == "1")
                f.setWeight(QFont::Bold);            // 75
            auto i = attrs.find("Italic");
            if (i != attrs.end() && i->second == "1")
                f.setStyle(QFont::StyleItalic);
        }
    }
    applyDpiScale(f);
    return f;
}

QFont KRTCreatorContext::GetFontSize(const KReportTemplateItem *pItem) const
{
    // Реф. @0x547690. (A) null/id не в конфиге → база+DPI; (B) FontType → рекурсия по
    // значению; (C) имя стиля по типу элемента.
    QFont f = baseReportFont();
    if (!pItem || !m_pData
        || m_pData->m_mapItemConfigs.find(pItem->m_strID) == m_pData->m_mapItemConfigs.end()) {
        applyDpiScale(f);
        return f;
    }
    const KReportTemplateItemConfig &cfg =
        m_pData->m_mapItemConfigs.at(pItem->m_strID);
    auto ft = cfg.m_mapAttrs.find("FontType");
    if (ft != cfg.m_mapAttrs.end())
        return GetFontSize(ft->second);              // (B)
    if (pItem->m_strType == "RT_TITLE_TABLE_BLOCK")
        return GetFontSize(std::string("FourthTitle"));
    if (pItem->m_strType == "RT_TEXT_BLOCK")
        return GetFontSize(std::string("ReportText"));
    if (pItem->m_strShowTitle == "1")                // item+0xc0 = m_strShowTitle
        return GetFontSize(std::string("FourthTitle"));
    return GetFontSize(std::string("ReportText"));
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

// --- KRTImageItemCreator: рендер блока одиночного изображения (реф. TU @0x535bd0) --------

bool KRTImageItemCreator::CreateBlock(KReportTemplateItem *pItem, QTextFrame *pFrame)
{
    // Реф. @0x536240: обёртка, как у текста. ElementId у KImageBlock и KTableBlock — один и
    // тот же item->m_strID, берём у модели изображения (KTableBlock не тянем).
    if (!pItem || !pFrame)
        return false;

    KImageBlock img(pItem, m_context.GetData());

    QTextCursor cur = pFrame->lastCursorPosition();
    cur.beginEditBlock();
    QTextFrameFormat frameFmt = pFrame->frameFormat();
    frameFmt.setProperty(QTextFormat::UserProperty + 1, QVariant(img.ElementId()));  // 0x100001
    QTextFrame *inner = cur.insertFrame(frameFmt);
    cur.endEditBlock();

    QTextCursor innerCur = inner->lastCursorPosition();
    const bool ok = renderImage(img, innerCur);

    m_context.HideInvalidBlock(inner);
    m_context.HideInvalidBlock(pFrame);
    return ok;
}

bool KRTImageItemCreator::renderImage(const KImageBlock &img, QTextCursor &cur)
{
    // Реф. @0x535c40. ГЕЙТ: файл картинки не загрузился → НИЧЕГО не вставляется (пустой
    // блок затем прячет HideInvalidBlock). Так off-device без картинок отчёт не падает.
    bool valid = false;
    const QString url = img.Url(valid);
    if (!valid)
        return true;

    QTextImageFormat fmt;
    fmt.setName(url);                              // ImageName 0x5000
    if (img.Heigth() > 0)                          // "Heigth" — опечатка реф., сохранена
        fmt.setHeight(img.Heigth());               // ImageHeight 0x5011
    if (img.Width() > 0)
        fmt.setWidth(img.Width());                 // ImageWidth 0x5010
    // Масштаб GetRatioTo1K НЕ применяется — размеры уже в device-px (в отличие от текста).

    QTextBlockFormat bfmt;
    // Выравнивание (реф. immediate-флаги; ЛЕВО/ПРАВО добавляют VCenter, ЦЕНТР — нет).
    const std::string a = img.GetAlign();
    if (a == "Left")
        bfmt.setAlignment(Qt::AlignVCenter | Qt::AlignLeft);    // 0x81
    else if (a == "Center")
        bfmt.setAlignment(Qt::AlignHCenter);                    // 0x04
    else if (a == "Right")
        bfmt.setAlignment(Qt::AlignVCenter | Qt::AlignRight);   // 0x82
    // нет совпадения → align не ставится (реф.)
    // Keep-маркер: image-блок НЕ прячется HideInvalidBlock (символ-замена «пуст» по тексту).
    bfmt.setProperty(QTextFormat::UserProperty + 2, QVariant(true));   // 0x100002

    cur.setBlockFormat(bfmt);
    cur.beginEditBlock();
    cur.insertBlock(bfmt);
    cur.insertImage(fmt);
    cur.endEditBlock();
    return true;
}

// --- KRTTextItemCreator: текст в ячейку таблицы (реф. @0x53b798) --------------

bool KRTTextItemCreator::CreateBlock(KReportTemplateItem *pItem, QTextTableCell &cell)
{
    // Реф. @0x53b798: как frame-обёртка, но БЕЗ вложенного фрейма — текст рисуется прямо
    // в курсор ячейки. Используется творцом таблиц для содержимого ячеек.
    if (!pItem || !cell.isValid())
        return false;
    KTextBlock block(pItem, m_context.GetData(), /*hideKey=*/false);
    QTextCursor cur = cell.lastCursorPosition();
    return renderBlock(block, cur);
}

// --- KRTTableItemCreator: рендер таблицы (реф. TU @0x539ab0) -------------------

bool KRTTableItemCreator::CreateBlock(KReportTemplateItem *pItem, QTextFrame *pFrame)
{
    // Реф. @0x53a400: строит QTextTable из KTableBlock и заполняет ячейки рекурсией.
    if (!pItem || !pFrame) {
        m_context.HideInvalidBlock(pFrame);
        return false;
    }
    KTableBlock blk(pItem, m_context.GetData());
    const QSize sz = blk.Size();               // width=строки, height=столбцы

    // Гейт реф.: есть строки ИЛИ есть заголовок. (Ветка single-column CreateFrame опущена —
    // всегда строим таблицу; для сетки 1 колонки это визуально эквивалентно, помечено.)
    if (sz.width() > 0 || blk.ShowTitle()) {
        QTextCursor cur = pFrame->lastCursorPosition();
        cur.beginEditBlock();
        QTextTable *t = createTable(cur, blk);
        cur.endEditBlock();
        if (!t) {
            m_context.HideInvalidBlock(pFrame);
            return false;
        }
        createChild(pItem->m_lstSubItems, t, blk.ShowTitle());
    }
    m_context.HideInvalidBlock(pFrame);
    return true;
}

QTextTable *KRTTableItemCreator::createTable(QTextCursor &cur, const KTableBlock &blk)
{
    // Реф. @0x53a6f0. МИНИМУМ: border/borderColor/colWidth + ElementId. Опущены (помечено)
    // cellSpacing, margins, хранение модели в UserProperty.
    QTextTableFormat fmt;
    fmt.setProperty(QTextFormat::UserProperty + 1, QVariant(blk.ElementId()));  // 0x100001
    fmt.setBorder(blk.BorderWidth());                       // FrameBorder 0x4000
    QColor bc;
    bc.setNamedColor(blk.BorderColor());                    // дефолт "black"
    if (bc.isValid())
        fmt.setBorderBrush(QBrush(bc, Qt::SolidPattern));   // FrameBorderBrush 0x4009
    const QVector<QTextLength> cw = blk.ColWidthContraints();
    if (!cw.isEmpty())
        fmt.setColumnWidthConstraints(cw);                  // 0x4101

    int rows = blk.Size().width();
    if (blk.ShowTitle())
        rows += 1;                                          // строка-заголовок сверху
    const int cols = blk.Size().height();
    if (rows <= 0 || cols <= 0)
        return nullptr;

    QTextTable *t = cur.insertTable(rows, cols, fmt);
    if (t && blk.ShowTitle()) {
        t->mergeCells(0, 0, 1, t->columns());               // объединить всю строку 0
        QTextTableCell c = t->cellAt(0, 0);
        if (c.isValid()) {
            QTextCursor cc = c.lastCursorPosition();
            insertTitle(cc, blk.TitleTextBlock());
        }
    }
    if (t)
        m_context.HideInvalidBlock(t);   // QTextTable — подкласс QTextFrame
    return t;
}

void KRTTableItemCreator::createChild(std::list<KReportTemplateItem> &items,
                                      QTextTable *pTable, bool hasTitle)
{
    // Реф. @0x539d08. МИНИМУМ: последовательная раскладка idx/cols × idx%cols; опущены
    // (помечено) cellat-переопределение позиции, ElementId на ячейках и removeRows-обрезка.
    if (!pTable)
        return;
    const int cols = pTable->columns();
    int idx = 0;
    for (KReportTemplateItem &node : items) {
        int row = cols ? idx / cols : 0;
        const int col = cols ? idx - row * cols : 0;
        if (hasTitle)
            row += 1;                                       // пропустить строку-заголовок
        if (row < pTable->rows() && col < pTable->columns()) {
            QTextTableCell cell = pTable->cellAt(row, col);
            if (cell.isValid()) {
                // Реф. GetCellWithID @0x539c20: штамп ElementId на формат ячейки (по нему
                // FindFrameOrCell/выделение находят ячейку-блок).
                QTextCharFormat cf = cell.format();
                cf.setProperty(QTextFormat::UserProperty + 1,
                               QVariant(QString::fromStdString(node.m_strID)));   // 0x100001
                cell.setFormat(cf);
                m_context.CreateBlock(node.m_strType, &node, cell);   // рекурсия в творцов
            }
        }
        ++idx;
    }
}

void KRTTableItemCreator::insertTitle(QTextCursor &cur, const KTextBlock &title)
{
    // Реф. InsertTitle @0x539448. МИНИМУМ: текст заголовка (полное форматирование —
    // char/block-формат — при развитии; на структуру таблицы не влияет).
    cur.insertText(title.FullText());
}
