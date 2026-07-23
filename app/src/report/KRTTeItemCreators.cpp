#include "report/KRTTeAbsItemCreator.h"

#include "report/KRTAbsDataSource.h"
#include "report/KRTTeCreatorContext.h"
#include "report/KReportDisplayParam.h"
#include "report/KReportTemplateCommonDef.h"

#include <QCoreApplication>
#include <QTextCursor>
#include <QTextTable>

#include "report/KMeaStringUtil.h"

#include <QColor>
#include <QTextImageFormat>

#include "ui/KScreenMng.h"

namespace {
// Реф. литералы (адреса сверены чтением .rodata).
const char *kTemplateData = "TemplateData";   // @0x865ca8 — ключ атрибута item-конфига
const char *kAlignH       = "AlignH";         // @0x865cb8
const char *kValue0       = "0";              // @0x874a58 — STR_RT_VALUE_0
const char *kValue2       = "2";              // @0x83fdf0 — STR_RT_VALUE_2
const char *kCenter       = "Center";         // @0x84d9b8
const char *kRight        = "Right";          // @0x89de00
const char *kTitleTail    = " :  ";           // @0x875108 — пробел, двоеточие, ДВА пробела

std::string Tr(const std::string &src)
{
    return QCoreApplication::translate("QObject", src.c_str()).toUtf8().constData();
}
}   // namespace

std::string KRTTeTextItemCreator::GetItemTitle(const KReportTemplateItem &item,
                                               const std::map<std::string, std::string> &) const
{
    // Реф. @0x51abd0.
    if (m_context.Mode() == 2)
        return std::string();                       // ранний выход, m_strTitle не читается
    if (item.m_strShowTitle.empty() || item.m_strShowTitle == kValue0)
        return std::string();                       // «не показывать заголовок»

    std::string title = Tr(item.m_strTitle);
    std::string real;
    if (report_template::QueryTemplateItemRealTitle(item, real))
        title = Tr(real);                           // ЗАМЕЩЕНИЕ, а не конкатенация
    // Реф. далее ветвится по IsGetFontChineseShow (билингвальный заголовок) —
    // эта ветка НЕ ВОССТАНОВЛЕНА и здесь не воспроизводится.
    return title;
}

int KRTTeTextItemCreator::CreateItem(const KReportTemplateItem &item,
                                     const std::map<std::string, std::string> &cfgMap,
                                     QTextTableCell &cell)
{
    // Реф. @0x51a160.
    if (!CheckCreate(item, cfgMap))
        return 0;                                    // единственный путь к false

    const std::string title = GetItemTitle(item, cfgMap);

    // Содержимое: из источника данных по m_strDataSrc (пусто → остаётся пустым).
    std::string content;
    KRTAbsDataSource *ds = m_context.DataSource();
    if (!item.m_strDataSrc.empty() && ds) {
        if (m_context.Mode() == 1 && !cfgMap.empty()) {
            std::string sid;
            if (report_template::ConvertToSourceID(item.m_strDataSrc, cfgMap, sid))
                ds->GetTextData(sid, content);
        } else {
            ds->GetTextData(item.m_strDataSrc, content);
        }
    }

    // Атрибуты текущего item-конфига.
    std::string alignH;
    if (const KReportTemplateItemConfig *cfg = CurItemConfig()) {
        auto td = cfg->m_mapAttrs.find(kTemplateData);
        if (td != cfg->m_mapAttrs.end()) {
            // Боковой механизм: значение атрибута ищется в конфигах шаблона внутри
            // KReportDisplayParam, и найденная строка через tr() ЗАМЕЩАЕТ содержимое.
            // Бизнес-смысл кросс-ссылки НЕ ВОССТАНОВЛЕН — воспроизведён факт вызовов.
            if (KReportDisplayParam *dp = m_context.DisplayParam()) {
                auto it = dp->TemplateConfigs().find(td->second);
                if (it != dp->TemplateConfigs().end() && !it->second.empty())
                    content = Tr(it->second);
            }
        }
        auto ah = cfg->m_mapAttrs.find(kAlignH);
        if (ah != cfg->m_mapAttrs.end())
            alignH = ah->second;
    }

    // ⚠️ m_strShowTitle == "2" ПРИНУДИТЕЛЬНО сбрасывает переопределение выравнивания.
    if (item.m_strShowTitle == kValue2)
        alignH.clear();

    QTextCursor c = cell.firstCursorPosition();
    c.beginEditBlock();

    QTextBlockFormat bf = c.blockFormat();
    if (alignH == kCenter)
        bf.setAlignment(Qt::AlignHCenter | Qt::AlignAbsolute);
    else if (alignH == kRight)
        bf.setAlignment(Qt::AlignRight | Qt::AlignAbsolute);
    else
        bf.setAlignment(Qt::AlignLeft | Qt::AlignAbsolute);
    c.setBlockFormat(bf);

    // Формат заголовка: размер из GetFontSize, жирность по IsPatientInfoTitleBold,
    // цвет из GetFontColor.
    QTextCharFormat titleFmt;
    QFont f = titleFmt.font();
    f.setPointSize(m_context.GetFontSize(&item));
    if (report_template::IsPatientInfoTitleBold(item))
        f.setWeight(75);                             // реф. константа (Bold)
    titleFmt.setFont(f);
    titleFmt.setForeground(QBrush(m_context.GetFontColor(&item)));

    if (!title.empty())
        c.insertText(QString::fromStdString(title + kTitleTail), titleFmt);

    // Содержимое — своим форматом (реф. повторно берёт размер и цвет).
    QTextCharFormat bodyFmt;
    QFont bf2 = bodyFmt.font();
    bf2.setPointSize(m_context.GetFontSize(&item));
    bodyFmt.setFont(bf2);
    bodyFmt.setForeground(QBrush(m_context.GetFontColor(&item)));
    c.insertText(QString::fromStdString(content), bodyFmt);

    c.endEditBlock();
    return 1;                                        // реф. на нормальном пути всегда true
}

// ── Остальные четыре: фильтр реф. соблюдён, содержимое НЕ строится ──
// Их тела (CreateChild @0x519430/@0x513b70/@0x515d90, CreateTable @0x5176f8,
// InsertTableTitle @0x5165c8, CalcmageWidth @0x514200) НЕ реверсированы.




// ─────────────────── Табличный творец (реф. @0x515e90 и далее) ───────────────────

namespace {
// Имена элементов, для которых CreateTable строит таблицу ВСЕГДА (реф. сравнения
// item.m_strName; адреса литералов сверены чтением .rodata).
const char *kNameHospitalOther = "HOSPITAL_OTHER";     // @0x875070
const char *kNameSignature     = "RT_SIGNATURE";       // @0x875080
const char *kNameAddition      = "RT_ADDITION";        // @0x875090
const char *kNameImageTextMap  = "RT_IMAGE_TEXT_MAP";  // @0x8750a0
const char *kValue1            = "1";                  // @0x885600 (хвост " ... 2>&1")

// ⭐ Ключи атрибутов item-конфига. Это НЕ литералы .rodata, а глобальные std::string,
// конструируемые динамическим инициализатором `_GLOBAL__sub_I_KRTAbsItemCreator.cpp`
// @0x2691a0 из общего строкового пула; таблица объектов — @0xa86098, шаг 0x20.
// Тексты восстановлены и сверены чтением .rodata (адреса ниже).
const char *kColumnRatio  = "ColumnRatio";   // @0x865c60 — «25,50,25»: доли столбцов
const char *kBorderWidth  = "BorderWidth";   // @0x865f10
const char *kBorderColor  = "BorderColor";   // @0x865f20
const char *kMarginWidth  = "MarginWidth";   // @0x865f30
// Остальные из той же таблицы (пока не используются здесь, зафиксированы в PROGRESS §10):
// CellAt @0x865d18, GroupingNum @0x865d38, MultiTitle @0x865d78, RefColumn @0x865c78,
// RefColumnID @0x865c88, BreakPolicy @0x865f40,
// LineHeight1/2/3/5 @0x865ec0/0x865ed0/0x865ee0/0x865f00 и LineTopMargin @0x865ef0
// (последний в реф. назван LINEHEGIHT4 — расхождение имени и значения у вендора).

std::string g_columnWidthAttrKey = kColumnRatio;
}   // namespace

void KRTTeTableItemCreator::SetColumnWidthAttrKey(const std::string &key)
{
    g_columnWidthAttrKey = key;
}

std::string KRTTeTableItemCreator::GetItemTitle(const KReportTemplateItem &item,
                                                const std::map<std::string, std::string> &) const
{
    // Реф. @0x5185a0.
    if (item.m_strShowTitle != kValue1)
        return std::string();           // гейт жёстче текстового творца: строго "1"
    if (m_context.Mode() == 2)
        return std::string();           // отдельная ветка реф. НЕ ВОССТАНОВЛЕНА
    std::string title = Tr(item.m_strTitle);
    if (title.empty()) {
        std::string real;
        if (report_template::QueryTemplateItemRealTitle(item, real))
            title = Tr(real);           // фолбэк при пустом переводе
    }
    return title;
}

void KRTTeTableItemCreator::InsertTableTitle(const KReportTemplateItem &item,
                                             const std::map<std::string, std::string> &cfgMap,
                                             QTextTableCell &cell)
{
    // Реф. @0x5165c8.
    const std::string title = GetItemTitle(item, cfgMap);
    QTextCursor c = cell.lastCursorPosition();
    c.beginEditBlock();
    if (title.empty()) {
        // Реф.: только настройка межстрочного интервала пустого блока (ключи атрибутов
        // и LineHeightType 2 vs 4 — НЕ ВОССТАНОВЛЕНЫ) и insertBlock.
        c.insertBlock();
        c.endEditBlock();
        return;
    }
    QTextCharFormat fmt;
    QFont f = fmt.font();
    f.setPointSize(m_context.GetFontSize(&item));
    fmt.setFont(f);
    fmt.setForeground(QBrush(m_context.GetFontColor(&item)));
    c.insertText(QString::fromStdString(title), fmt);
    c.endEditBlock();
}

QTextTable *KRTTeTableItemCreator::CreateTable(const KReportTemplateItem &item,
                                               const std::map<std::string, std::string> &,
                                               QTextTableCell &cell)
{
    // Реф. @0x5176f8. Столбцы — из m_strColumn.
    const int cols = QString::fromStdString(item.m_strColumn).toInt();

    const bool alwaysBuild = item.m_strName == kNameHospitalOther
                             || item.m_strName == kNameSignature
                             || item.m_strName == kNameAddition
                             || item.m_strName == kNameImageTextMap;
    if (!alwaysBuild && cols <= 1)
        return nullptr;                 // реф.: дети пойдут прямо в исходную ячейку

    const int n = static_cast<int>(item.m_lstSubItems.size());
    // ⚠️ Реф. считает число строк по невосстановленной цепочке (поле item+0xf0 и
    // «строк в заголовке» из под-конфига). Здесь — прямая раскладка детей по сетке.
    const int rows = (n > 0 && cols > 0) ? (n + cols - 1) / cols : 1;

    QTextTableFormat fmt;
    fmt.setCellPadding(0.0);            // реф. property 0x4103 = 0.0
    // Рамка и поля — из атрибутов конфига (ключи восстановлены, см. выше).
    if (const KReportTemplateItemConfig *cfg = CurItemConfig()) {
        auto bw = cfg->m_mapAttrs.find(kBorderWidth);
        if (bw != cfg->m_mapAttrs.end())
            fmt.setBorder(QString::fromStdString(bw->second).toDouble());
        auto bc = cfg->m_mapAttrs.find(kBorderColor);
        if (bc != cfg->m_mapAttrs.end()) {
            QColor col;
            col.setNamedColor(QString::fromStdString(bc->second));
            if (col.isValid())
                fmt.setBorderBrush(QBrush(col));
        }
        auto mw = cfg->m_mapAttrs.find(kMarginWidth);
        if (mw != cfg->m_mapAttrs.end())
            fmt.setMargin(QString::fromStdString(mw->second).toDouble());
    }
    QVector<QTextLength> widths;
    // Список ширин — строка «25,50,25», режется по "," (@0x863930) и переводится
    // в проценты; ключ атрибута НЕ ВОССТАНОВЛЕН ⇒ по умолчанию равные доли.
    std::string spec;
    if (!g_columnWidthAttrKey.empty()) {
        if (const KReportTemplateItemConfig *cfg = CurItemConfig()) {
            auto it = cfg->m_mapAttrs.find(g_columnWidthAttrKey);
            if (it != cfg->m_mapAttrs.end())
                spec = it->second;
        }
    }
    if (!spec.empty()) {
        KMeaStringUtil util;
        for (const std::string &tok : util.SplitStr(spec, ","))
            widths << QTextLength(QTextLength::PercentageLength,
                                  QString::fromStdString(tok).toFloat());
    }
    if (widths.size() != cols) {
        widths.clear();
        for (int i = 0; i < cols; ++i)
            widths << QTextLength(QTextLength::PercentageLength, 100.0 / cols);
    }
    fmt.setColumnWidthConstraints(widths);

    QTextCursor c = cell.lastCursorPosition();
    return c.insertTable(rows, cols, fmt);
}

int KRTTeTableItemCreator::CreateChild(const std::list<KReportTemplateItem> &items,
                                       const std::map<std::string, std::string> &cfgMap,
                                       QTextTableCell &cell)
{
    // Реф. @0x515d90 — линейно, параграфами в одну ячейку.
    QTextCursor tail = cell.lastCursorPosition();
    if (items.empty()) {
        tail.deletePreviousChar();
        return 0;
    }
    int total = 0;
    for (const KReportTemplateItem &sub : items) {
        QTextCursor c = cell.lastCursorPosition();
        c.beginEditBlock();
        const int n = m_context.CreateItem(sub, cfgMap, cell);
        if (n > 0) {
            total += n;
            c = cell.lastCursorPosition();
            c.insertBlock();
        }
        c.endEditBlock();
    }
    cell.lastCursorPosition().deletePreviousChar();   // убрать хвостовой перевод блока
    return total;
}

int KRTTeTableItemCreator::CreateChild(const std::list<KReportTemplateItem> &items,
                                       const std::map<std::string, std::string> &cfgMap,
                                       QTextTable *table)
{
    // Реф. @0x5160c8 — раскладка по сетке + подрезка лишних строк.
    if (!table)
        return 0;
    const int cols = table->columns();
    int total = 0, i = 0, usedRows = 0;
    for (const KReportTemplateItem &sub : items) {
        const int row = cols > 0 ? i / cols : 0;
        const int col = cols > 0 ? i % cols : 0;
        if (row >= table->rows())
            break;
        QTextTableCell cell = table->cellAt(row, col);
        const int n = m_context.CreateItem(sub, cfgMap, cell);
        if (n > 0)
            total += n;
        usedRows = row + 1;
        ++i;
    }
    if (usedRows < table->rows())
        table->removeRows(usedRows, table->rows() - usedRows);
    return total;
}

int KRTTeTableItemCreator::CreateItem(const KReportTemplateItem &item,
                                      const std::map<std::string, std::string> &cfgMap,
                                      QTextTableCell &cell)
{
    // Реф. @0x515ec0.
    if (!CheckCreate(item, cfgMap))
        return 0;

    InsertTableTitle(item, cfgMap, cell);            // безусловно, до таблицы

    QTextTable *table = CreateTable(item, cfgMap, cell);
    if (!table)
        return CreateChild(item.m_lstSubItems, cfgMap, cell);   // ВЫХОД без разделителя

    int n = 0;
    if (table->rows() == 1 && table->columns() == 1) {
        QTextTableCell c0 = table->cellAt(0, 0);
        n = CreateChild(item.m_lstSubItems, cfgMap, c0);
    } else {
        n = CreateChild(item.m_lstSubItems, cfgMap, table);
    }

    if (KReportDisplayParam *dp = m_context.DisplayParam()) {
        KSplitLineInfo info;
        report_template::GetSplitLineInfo(item.m_strID, dp->ItemConfigs(), info);
        if (info.m_nSplitLineWidth > 0)
            m_context.InsertSplitLine(info, table);
    }
    return n;
}

// ─────────── Оставшиеся четыре творца (реф. тела) ───────────

namespace {
// ⚠️ РАЗНЫЕ разделители у разных творцов — проверено чтением .rodata:
const char *kSepTextGroup = " : ";    // @0x8750e8 — ТРИ символа (у текстового творца ЧЕТЫРЕ)
const char *kNoPFData     = "NoPFData";   // @0x874e10
std::string g_imgWidthKey, g_imgHeightKey, g_imgAlignKey;
}   // namespace

// ── RT_TEXTGROUP_BLOCK (реф. @0x519a78) ──

int KRTTeTextGroupCreator::CreateChild(
    const std::vector<std::pair<std::string, std::string>> &pairs, QTextTableCell &cell)
{
    // Реф. @0x519430: всё в одну ячейку, БЕЗ явного char-формата.
    QTextCursor c = cell.lastCursorPosition();
    for (const auto &kv : pairs)
        c.insertText(QString::fromStdString(kv.first + kSepTextGroup + kv.second));
    return static_cast<int>(pairs.size());
}

int KRTTeTextGroupCreator::CreateChild(
    const std::vector<std::pair<std::string, std::string>> &pairs, QTextTable *table)
{
    // Реф. @0x5196c0: resize под число пар, раскладка по сетке и char-формат из
    // GetFontSize/GetFontColor с item == nullptr (дефолтный шрифт, НЕ item-специфичный).
    if (!table)
        return 0;
    const int cols = table->columns() > 0 ? table->columns() : 1;
    const int rows = (static_cast<int>(pairs.size()) + cols - 1) / cols;
    if (rows > 0 && table->rows() != rows)
        table->resize(rows, cols);

    QTextCharFormat fmt;
    QFont f = fmt.font();
    f.setPointSize(m_context.GetFontSize(nullptr));
    fmt.setFont(f);
    fmt.setForeground(QBrush(m_context.GetFontColor(nullptr)));

    int i = 0;
    for (const auto &kv : pairs) {
        const int r = i / cols, cl = i % cols;
        if (r >= table->rows())
            break;
        QTextTableCell cell = table->cellAt(r, cl);
        QTextCursor c = cell.lastCursorPosition();
        c.insertText(QString::fromStdString(kv.first + kSepTextGroup + kv.second), fmt);
        ++i;
    }
    return static_cast<int>(pairs.size());
}

int KRTTeTextGroupCreator::CreateItem(const KReportTemplateItem &item,
                                      const std::map<std::string, std::string> &cfgMap,
                                      QTextTableCell &cell)
{
    if (!CheckCreate(item, cfgMap))
        return 0;
    KRTAbsDataSource *ds = m_context.DataSource();
    std::vector<std::pair<std::string, std::string>> pairs;
    std::string id = item.m_strDataSrc;
    if (!cfgMap.empty()) {
        std::string sid;
        if (report_template::ConvertToSourceID(item.m_strDataSrc, cfgMap, sid))
            id = sid;
    }
    if (!ds || !ds->GetTextGroupData(id, pairs))   // слот 5
        return 0;

    KRTTeTableItemCreator helper(m_context);       // реф. зовёт базовые табличные слоты
    helper.InsertTableTitle(item, cfgMap, cell);
    QTextTable *table = helper.CreateTable(item, cfgMap, cell);
    if (!table)
        return CreateChild(pairs, cell);
    if (table->rows() == 1 && table->columns() == 1) {
        QTextTableCell c0 = table->cellAt(0, 0);
        return CreateChild(pairs, c0);
    }
    return CreateChild(pairs, table);
}

// ── RT_IMAGE_BLOCK (реф. @0x514870) ──

void KRTTeImageItemCreator::SetImageAttrKeys(const std::string &w, const std::string &h,
                                             const std::string &a)
{
    g_imgWidthKey = w; g_imgHeightKey = h; g_imgAlignKey = a;
}

std::string KRTTeImageItemCreator::GetItemTitle(const KReportTemplateItem &item,
                                                const std::map<std::string, std::string> &) const
{
    // Реф. @0x515270 — два гейта, оба ведут к ПУСТОЙ строке.
    if (m_context.Mode() == 2)
        return std::string();
    if (item.m_strShowTitle != kValue1)
        return std::string();
    return Tr(item.m_strTitle);
}

int KRTTeImageItemCreator::CreateItem(const KReportTemplateItem &item,
                                      const std::map<std::string, std::string> &cfgMap,
                                      QTextTableCell &cell)
{
    // Реф. @0x514870.
    if (!CheckCreate(item, cfgMap))
        return 0;

    std::string path = item.m_strDataSrc;
    if (!cfgMap.empty()) {
        std::string sid;
        if (report_template::ConvertToSourceID(item.m_strDataSrc, cfgMap, sid))
            path = sid;
    }

    // Размеры: значение атрибута ДЕЛИТСЯ на KScreenMng::GetRatioTo1K() — масштабирование
    // под текущее разрешение. Имена ключей ширины/высоты/выравнивания НЕ ВОССТАНОВЛЕНЫ
    // (статические std::string в .bss ДРУГОЙ таблицы) ⇒ настраиваемые, по умолчанию пусты.
    const double ratio = KScreenMng::GetInstance()->GetRatioTo1K();
    QTextImageFormat img;
    img.setName(QString::fromStdString(path));
    std::string alignVal;
    if (const KReportTemplateItemConfig *cfg = CurItemConfig()) {
        auto get = [&](const std::string &k) -> std::string {
            if (k.empty()) return std::string();
            auto it = cfg->m_mapAttrs.find(k);
            return it == cfg->m_mapAttrs.end() ? std::string() : it->second;
        };
        const std::string w = get(g_imgWidthKey), h = get(g_imgHeightKey);
        alignVal = get(g_imgAlignKey);
        if (!w.empty() && ratio != 0.0)
            img.setWidth(QString::fromStdString(w).toDouble() / ratio);
        if (!h.empty() && ratio != 0.0)
            img.setHeight(QString::fromStdString(h).toDouble() / ratio);
    }

    QTextCursor c = cell.lastCursorPosition();
    QTextBlockFormat bf = c.blockFormat();
    if (alignVal == "Left")
        bf.setAlignment(Qt::AlignLeft | Qt::AlignAbsolute);
    else if (alignVal == "Right")
        bf.setAlignment(Qt::AlignRight | Qt::AlignAbsolute);
    else
        bf.setAlignment(Qt::AlignHCenter);     // реф. значение 4 — центр по умолчанию
    c.setBlockFormat(bf);

    // Заголовок вставляется ПЕРЕД картинкой, но НЕ при значении "NoPFData".
    const std::string title = GetItemTitle(item, cfgMap);
    if (!title.empty() && alignVal != kNoPFData) {
        QTextCharFormat tf;
        QFont f = tf.font();
        f.setPointSize(m_context.GetFontSize(&item));
        tf.setFont(f);
        tf.setForeground(QBrush(m_context.GetFontColor(&item)));
        c.insertText(QString::fromStdString(title), tf);
    }
    c.insertImage(img);
    return 1;
}

// ── RT_IMAGEGROUP_BLOCK (реф. @0x514410) ──

float KRTTeImageGroupCreator::CalcmageWidth(QTextTable *table) const
{
    // Реф. @0x514200: 600.0 / columns() по умолчанию (реф. имеет assert(columns > 0)).
    // Атрибут-переопределитель существует, но его КЛЮЧ НЕ ВОССТАНОВЛЕН.
    const int cols = (table && table->columns() > 0) ? table->columns() : 1;
    return 600.0f / cols;
}

int KRTTeImageGroupCreator::CreateChild(
    const std::vector<std::pair<std::string, std::string>> &pairs, float width,
    QTextTableCell &cell)
{
    // Реф. @0x513b70: все картинки подряд в ОДНУ ячейку.
    QTextCursor c = cell.lastCursorPosition();
    for (const auto &kv : pairs) {
        QTextImageFormat img;
        img.setName(QString::fromStdString(kv.second));
        if (width > 0)
            img.setWidth(width);
        c.insertImage(img);
    }
    return static_cast<int>(pairs.size());
}

int KRTTeImageGroupCreator::CreateChild(
    const std::vector<std::pair<std::string, std::string>> &pairs, float width,
    QTextTable *table)
{
    // Реф. @0x513ea0: resize под ceil(count/cols), раскладка по сетке.
    if (!table)
        return 0;
    const int cols = table->columns() > 0 ? table->columns() : 1;
    const int rows = (static_cast<int>(pairs.size()) + cols - 1) / cols;
    if (rows > 0 && table->rows() != rows)
        table->resize(rows, cols);
    int i = 0;
    for (const auto &kv : pairs) {
        const int r = i / cols, cl = i % cols;
        if (r >= table->rows())
            break;
        QTextTableCell cell = table->cellAt(r, cl);
        QTextCursor c = cell.lastCursorPosition();
        QTextImageFormat img;
        img.setName(QString::fromStdString(kv.second));
        if (width > 0)
            img.setWidth(width);
        c.insertImage(img);
        ++i;
    }
    return static_cast<int>(pairs.size());
}

int KRTTeImageGroupCreator::CreateItem(const KReportTemplateItem &item,
                                       const std::map<std::string, std::string> &cfgMap,
                                       QTextTableCell &cell)
{
    if (!CheckCreate(item, cfgMap))
        return 0;
    KRTAbsDataSource *ds = m_context.DataSource();
    std::vector<std::pair<std::string, std::string>> pairs;
    std::string id = item.m_strDataSrc;
    if (!cfgMap.empty()) {
        std::string sid;
        if (report_template::ConvertToSourceID(item.m_strDataSrc, cfgMap, sid))
            id = sid;
    }
    if (ds)
        ds->GetImageGroupData(id, pairs);          // слот 7

    KRTTeTableItemCreator helper(m_context);
    helper.InsertTableTitle(item, cfgMap, cell);
    QTextTable *table = helper.CreateTable(item, cfgMap, cell);
    const float w = CalcmageWidth(table);
    if (!table) {
        CreateChild(pairs, w, cell);
        return 1;
    }
    if (table->rows() == 1 && table->columns() == 1) {
        QTextTableCell c0 = table->cellAt(0, 0);
        CreateChild(pairs, w, c0);
    } else {
        CreateChild(pairs, w, table);
    }
    return 1;
}

// ── RT_SUB_DATA_BLOCK (реф. @0x515510) ──

int KRTTeSubDataItemCreator::CreateItem(const KReportTemplateItem &item,
                                        const std::map<std::string, std::string> &cfgMap,
                                        QTextTableCell &cell)
{
    // Реф. @0x515510.
    if (!CheckCreate(item, cfgMap))
        return 0;
    KRTAbsDataSource *ds = m_context.DataSource();
    std::string id = item.m_strDataSrc;
    if (!cfgMap.empty()) {
        std::string sid;
        if (report_template::ConvertToSourceID(item.m_strDataSrc, cfgMap, sid))
            id = sid;
    }
    KReportTemplateDataNew sub;
    if (!ds || !ds->GetSubData(id, sub))           // слот 8
        return 0;

    KRTTeTableItemCreator helper(m_context);
    helper.InsertTableTitle(item, cfgMap, cell);

    if (sub.m_lstItems.empty() && m_context.Mode() != 2) {
        // Пусто и не режим 2: при ShowTitle == "1" — просто успех без контента,
        // иначе вставляется ОДИН блок с m_strTitle как обычный текст.
        if (item.m_strShowTitle == kValue1)
            return 1;
        QTextCursor c = cell.lastCursorPosition();
        QTextCharFormat tf;
        QFont f = tf.font();
        f.setPointSize(m_context.GetFontSize(&item));
        tf.setFont(f);
        tf.setForeground(QBrush(m_context.GetFontColor(&item)));
        c.insertText(QString::fromStdString(item.m_strTitle), tf);
        return 1;
    }

    // Конфиги вложенного шаблона регистрируются в KReportDisplayParam (merge).
    if (KReportDisplayParam *dp = m_context.DisplayParam())
        dp->AppendItemParam(sub.m_mapItemConfigs);

    QTextTable *table = helper.CreateTable(item, cfgMap, cell);
    if (!table) {
        helper.CreateChild(sub.m_lstItems, cfgMap, cell);
    } else if (table->rows() == 1 && table->columns() == 1) {
        QTextTableCell c0 = table->cellAt(0, 0);
        helper.CreateChild(sub.m_lstItems, cfgMap, c0);
    } else {
        helper.CreateChild(sub.m_lstItems, cfgMap, table);
    }
    return 1;
}
