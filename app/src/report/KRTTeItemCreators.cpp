#include "report/KRTTeAbsItemCreator.h"

#include "report/KRTAbsDataSource.h"
#include "report/KRTTeCreatorContext.h"
#include "report/KReportDisplayParam.h"
#include "report/KReportTemplateCommonDef.h"

#include <QCoreApplication>
#include <QTextCursor>
#include <QTextTable>

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

int KRTTeTextGroupCreator::CreateItem(const KReportTemplateItem &item,
                                      const std::map<std::string, std::string> &cfgMap,
                                      QTextTableCell &)
{
    return CheckCreate(item, cfgMap) ? 0 : 0;   // реф. @0x519a78 — не портирован
}

int KRTTeImageItemCreator::CreateItem(const KReportTemplateItem &item,
                                      const std::map<std::string, std::string> &cfgMap,
                                      QTextTableCell &)
{
    return CheckCreate(item, cfgMap) ? 0 : 0;   // реф. @0x514870 — не портирован
}

int KRTTeImageGroupCreator::CreateItem(const KReportTemplateItem &item,
                                       const std::map<std::string, std::string> &cfgMap,
                                       QTextTableCell &)
{
    return CheckCreate(item, cfgMap) ? 0 : 0;   // реф. @0x514410 — не портирован
}

int KRTTeTableItemCreator::CreateItem(const KReportTemplateItem &item,
                                      const std::map<std::string, std::string> &cfgMap,
                                      QTextTableCell &)
{
    return CheckCreate(item, cfgMap) ? 0 : 0;   // реф. @0x515ec0 — не портирован
}

int KRTTeSubDataItemCreator::CreateItem(const KReportTemplateItem &item,
                                        const std::map<std::string, std::string> &cfgMap,
                                        QTextTableCell &)
{
    return CheckCreate(item, cfgMap) ? 0 : 0;   // реф. @0x515510 — не портирован
}
