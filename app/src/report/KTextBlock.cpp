#include "report/KTextBlock.h"
#include "report/KReportTemplateCommonDef.h"
#include "report/KMeaStringUtil.h"

#include <QObject>

namespace {
// Ключи атрибутов (реф. report_template::STR_RT_ITEM_ATTR_* / STR_FONT_ATTR_*).
const char *ATTR_TEMPLATE_DATA = "TemplateData";
const char *ATTR_FONT_TYPE     = "FontType";
const char *ATTR_SIZE          = "Size";
const char *ATTR_BOLD          = "Bold";
const char *ATTR_ITALIC        = "Italic";
const char *ATTR_ALIGN_H       = "AlignH";
const char *VALUE_TRUE         = "1";
const char *ALIGN_CENTER       = "Center";
const char *ALIGN_RIGHT        = "Right";
} // namespace

KTextBlock::KTextBlock(KReportTemplateItem *item, KReportTemplateDataNew *dataNew, bool hideKey)
    : m_pTemplateItem(item), m_pDataNew(dataNew), m_bHideKey(hideKey)
{
    InitItemConfig();
}

void KTextBlock::InitItemConfig()
{
    if (!m_pTemplateItem || !m_pDataNew)
        return;
    // Кэшируем per-item стиль-конфиг по ID (реф.: m_pDataNew@0x48[m_strID]).
    const auto it = m_pDataNew->m_mapItemConfigs.find(m_pTemplateItem->m_strID);
    if (it != m_pDataNew->m_mapItemConfigs.end())
        m_itemConfig = it->second;
    // иначе реф. логирует "invalid KTextBlock! init item config failed" — m_itemConfig пуст.
}

std::string KTextBlock::attr(const std::string &key) const
{
    const auto it = m_itemConfig.m_mapAttrs.find(key);
    return it != m_itemConfig.m_mapAttrs.end() ? it->second : std::string();
}

std::string KTextBlock::styleAttr(const std::string &key) const
{
    if (!m_pDataNew)
        return std::string();
    // item-config → FontType → стиль-конфиг в m_mapItemConfigs → атрибут key.
    const std::string fontType = attr(ATTR_FONT_TYPE);
    if (fontType.empty())
        return std::string();
    const auto s = m_pDataNew->m_mapItemConfigs.find(fontType);
    if (s == m_pDataNew->m_mapItemConfigs.end())
        return std::string();
    const auto a = s->second.m_mapAttrs.find(key);
    return a != s->second.m_mapAttrs.end() ? a->second : std::string();
}

QString KTextBlock::ElementId() const
{
    return m_pTemplateItem ? QString::fromStdString(m_pTemplateItem->m_strID) : QString();
}

std::string KTextBlock::Data() const
{
    // TemplateData-атрибут → ключ → значение в m_mapConfigs → tr().
    const std::string key = attr(ATTR_TEMPLATE_DATA);
    std::string res;
    if (!key.empty() && m_pDataNew) {
        const auto it = m_pDataNew->m_mapConfigs.find(key);
        if (it != m_pDataNew->m_mapConfigs.end())
            res = it->second;
    }
    return QObject::tr(res.c_str()).toUtf8().constData();
}

std::string KTextBlock::Title() const
{
    if (!m_pTemplateItem)
        return std::string();
    const std::string raw = report_template::QueryTemplateItemRealTitle(*m_pTemplateItem);
    const std::string t = QObject::tr(raw.c_str()).toUtf8().constData();
    // Реф.: если заголовок непуст и не совпадает с Data → "title : ".
    if (t.empty() || t == Data())
        return std::string();
    KMeaStringUtil u;
    return u.FormatStr("%s : ", t.c_str());
}

QString KTextBlock::FullText() const
{
    return QString::fromStdString(Title()).append(QString::fromStdString(Data()));
}

bool KTextBlock::Bold() const
{
    return styleAttr(ATTR_BOLD) == VALUE_TRUE;
}

bool KTextBlock::Italic() const
{
    return styleAttr(ATTR_ITALIC) == VALUE_TRUE;
}

int KTextBlock::FontSize(int &out) const
{
    const std::string s = styleAttr(ATTR_SIZE);
    KMeaStringUtil u;
    out = u.ConvertStringToInt(s);   // pt (DPI-масштаб pt→px опущен — device-рендер)
    return out;
}

bool KTextBlock::Alignment(QFlags<Qt::AlignmentFlag> &out) const
{
    const std::string a = attr(ATTR_ALIGN_H);
    if (a == ALIGN_CENTER)
        out = Qt::AlignHCenter;
    else if (a == ALIGN_RIGHT)
        out = Qt::AlignRight;
    else
        out = Qt::AlignLeft;
    return !a.empty();   // задан ли атрибут явно
}

bool KTextBlock::LineHeight(int n, int &out) const
{
    const std::string key = "LineHeight" + std::to_string(n);
    const std::string v = attr(key);
    if (v.empty())
        return false;
    out = QString::fromStdString(v).toInt();
    return true;
}
