#include "report/KTableBlock.h"
#include "report/KMeaStringUtil.h"

#include <QString>

namespace {
// Ключи item-config-атрибутов (реф. report_template::STR_RT_ITEM_ATTR_* +
// инлайн-литералы). Резолв — из static-init TU + .rodata.
const char *ATTR_REF_COLUMN_ID = "RefColumnID";
const char *ATTR_REF_COLUMN    = "RefColumn";
const char *ATTR_BORDER_WIDTH  = "BorderWidth";
const char *ATTR_MARGIN_WIDTH  = "MarginWidth";   // sic — не "Margin"
const char *ATTR_BORDER_COLOR  = "BorderColor";
const char *ATTR_COLUMN_RATIO  = "ColumnRatio";   // инлайн-литерал, не STR_* глобал
const char *DEFAULT_FG_COLOR   = "black";         // STR_DEFULT_FOREGROUND_COLOR
const char *VALUE_TRUE         = "1";

// Тип узла RT_TITLE_TABLE_BLOCK и 7 типов, которые TableType() к нему ремапит.
const char *TYPE_TITLE_TABLE = "RT_TITLE_TABLE_BLOCK";
const char *TITLE_TABLE_ALIASES[] = {
    "RT_OB_Z_SCORE_BLOCK", "RT_WMS_BLOCK", "RT_SEWMS_BLOCK", "RT_OB_EFW_BLOCK",
    "RT_SUB_DATA_BLOCK", "RT_TEXTGROUP_BLOCK", "RT_IMAGEGROUP_BLOCK",
};

const double MARGIN_SCALE = 0.62;   // реф. hardcoded (rodata)
} // namespace

KTableBlock::KTableBlock(KReportTemplateItem *item, KReportTemplateDataNew *dataNew)
    : m_pTemplateItem(item), m_pDataNew(dataNew)
{
    InitItemConfig();
    CalcTableSize();
}

void KTableBlock::InitItemConfig()
{
    if (!m_pTemplateItem || !m_pDataNew)
        return;
    const auto it = m_pDataNew->m_mapItemConfigs.find(m_pTemplateItem->m_strID);
    if (it != m_pDataNew->m_mapItemConfigs.end())
        m_itemConfig = it->second;
}

std::string KTableBlock::attr(const std::string &key) const
{
    const auto it = m_itemConfig.m_mapAttrs.find(key);
    return it != m_itemConfig.m_mapAttrs.end() ? it->second : std::string();
}

void KTableBlock::CalcTableSize()
{
    if (!m_pTemplateItem || !m_pDataNew)
        return;

    // Число столбцов: атрибут Column узла Content; переопределяется RefColumnID→RefColumn.
    int nCol = QString::fromStdString(m_pTemplateItem->m_strColumn).toInt();

    const auto refIdIt = m_itemConfig.m_mapAttrs.find(ATTR_REF_COLUMN_ID);
    if (refIdIt != m_itemConfig.m_mapAttrs.end()) {
        // реф. operator[] — вставит пустой конфиг, если refName нет в карте.
        const KReportTemplateItemConfig &ref = m_pDataNew->m_mapItemConfigs[refIdIt->second];
        const auto refColIt = ref.m_mapAttrs.find(ATTR_REF_COLUMN);
        if (refColIt != ref.m_mapAttrs.end())
            nCol = QString::fromStdString(refColIt->second).toInt();
    }

    const int childCount = static_cast<int>(m_pTemplateItem->m_lstSubItems.size());
    if (nCol > 0) {
        int rows = childCount / nCol;
        if (childCount % nCol != 0)
            ++rows;
        m_size = QSize(rows, nCol);      // width=строки, height=столбцы
    } else {
        m_size = QSize(childCount, 1);
    }
}

QSize KTableBlock::Size() const
{
    return m_size;
}

bool KTableBlock::GetTemplateItemForCell(int row, int col, KReportTemplateItem *&out) const
{
    if (row < 0 || col < 0)
        return false;   // реф. — printf ошибки + false, out не трогается

    const int cols = m_size.height();
    const int index = row * cols + col;   // row-major

    auto &list = m_pTemplateItem->m_lstSubItems;
    if (index < 0 || index >= static_cast<int>(list.size()))
        return false;

    auto it = list.begin();
    std::advance(it, index);
    out = &(*it);
    return true;
}

QString KTableBlock::ElementId() const
{
    // реф. — прямой дереференс без null-guard (отличие от KTextBlock/KImageBlock).
    return QString::fromLatin1(m_pTemplateItem->m_strID.c_str());
}

bool KTableBlock::ShowTitle() const
{
    // Читает ПОЛЕ узла (ShowTitle-строка @0xC0), а не item-config-атрибут.
    return m_pTemplateItem->m_strShowTitle == VALUE_TRUE;
}

KTextBlock KTableBlock::TitleTextBlock() const
{
    return KTextBlock(m_pTemplateItem, m_pDataNew, /*hideKey=*/true);
}

QString KTableBlock::TableType() const
{
    const std::string &type = m_pTemplateItem->m_strType;
    for (const char *alias : TITLE_TABLE_ALIASES) {
        if (type == alias)
            return QString::fromLatin1(TYPE_TITLE_TABLE);
    }
    return QString::fromLatin1(type.c_str());
}

float KTableBlock::BorderWidth() const
{
    const std::string s = attr(ATTR_BORDER_WIDTH);
    if (s.empty())
        return 0.0f;
    return QString::fromLatin1(s.c_str()).toFloat();
}

bool KTableBlock::ShowBorder() const
{
    return BorderWidth() != 0.0;   // реф. !math::IsEqual(BorderWidth(), 0.0)
}

std::vector<float> KTableBlock::Margin() const
{
    std::vector<float> res;
    const std::string s = attr(ATTR_MARGIN_WIDTH);
    if (s.empty())
        return res;
    KMeaStringUtil u;
    for (const std::string &tok : u.SplitStr(s, ",")) {
        const float v = static_cast<float>(QString::fromLatin1(tok.c_str()).toFloat() * MARGIN_SCALE);
        res.push_back(v);
    }
    return res;
}

QString KTableBlock::BorderColor() const
{
    // реф.: дефолт "black", перезаписывается значением атрибута ПРИ НАЛИЧИИ ключа
    // (даже пустым) — потому find(), а не attr()/проверка на пусто.
    std::string s = DEFAULT_FG_COLOR;
    const auto it = m_itemConfig.m_mapAttrs.find(ATTR_BORDER_COLOR);
    if (it != m_itemConfig.m_mapAttrs.end())
        s = it->second;
    return QString::fromLatin1(s.c_str());
}

QVector<QTextLength> KTableBlock::ColWidthContraints() const
{
    QVector<QTextLength> res;
    const std::string s = attr(ATTR_COLUMN_RATIO);
    if (s.empty())
        return res;
    KMeaStringUtil u;
    for (const std::string &tok : u.SplitStr(s, ",")) {
        const int pct = QString::fromLatin1(tok.c_str()).toInt();
        res.append(QTextLength(QTextLength::PercentageLength, static_cast<qreal>(pct)));
    }
    return res;
}
