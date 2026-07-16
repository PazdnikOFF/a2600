#include "report/KImageBlock.h"
#include "report/KReportTemplateCommonDef.h"
#include "report/KMeaStringUtil.h"

#include <QImage>
#include <QObject>

namespace {
const char *ATTR_IMAGE_WIDTH  = "ImageWidth";
const char *ATTR_IMAGE_HEIGHT = "ImageHeight";
const char *ATTR_ALIGN_H      = "AlignH";

// Реф. MAP_LOCAL_PIC_PFINFO — глобальный .bss-кэш source-id → путь картинки.
std::map<std::string, std::string> g_picMap;
} // namespace

void KImageBlock::RegisterPicPath(const std::string &sourceId, const std::string &path)
{
    g_picMap[sourceId] = path;
}

void KImageBlock::ClearPicMap()
{
    g_picMap.clear();
}

KImageBlock::KImageBlock(KReportTemplateItem *item, KReportTemplateDataNew *dataNew)
    : m_pTemplateItem(item), m_pDataNew(dataNew)
{
    InitItemConfig();
}

void KImageBlock::InitItemConfig()
{
    if (!m_pTemplateItem || !m_pDataNew)
        return;
    const auto it = m_pDataNew->m_mapItemConfigs.find(m_pTemplateItem->m_strID);
    if (it != m_pDataNew->m_mapItemConfigs.end())
        m_itemConfig = it->second;
}

std::string KImageBlock::attr(const std::string &key) const
{
    const auto it = m_itemConfig.m_mapAttrs.find(key);
    return it != m_itemConfig.m_mapAttrs.end() ? it->second : std::string();
}

QString KImageBlock::ElementId() const
{
    return m_pTemplateItem ? QString::fromStdString(m_pTemplateItem->m_strID) : QString();
}

QString KImageBlock::ImageName() const
{
    // реф. — tr от Name узла (@0x20), а не Title.
    return m_pTemplateItem ? QObject::tr(m_pTemplateItem->m_strName.c_str()) : QString();
}

QString KImageBlock::Url(bool &valid) const
{
    valid = false;
    if (!m_pTemplateItem)
        return QString();

    std::string str = m_pTemplateItem->m_strDataSrc;
    if (!m_mapTextParam.empty())
        str = report_template::ConvertToSourceID(str, m_mapTextParam);

    KMeaStringUtil u;
    const std::vector<std::string> tokens = u.SplitStr(str, ",");
    if (tokens.empty())
        return QString();
    const std::string key = tokens.back();   // ПОСЛЕДНИЙ токен

    // реф. operator[] — вставка пустого пути, если ключа нет.
    const std::string path = g_picMap[key];
    const QString qpath = QString::fromLatin1(path.c_str());
    valid = !QImage(qpath).isNull();   // bool& = файл загрузился как картинка
    return qpath;                       // путь возвращается ВСЕГДА
}

int KImageBlock::Width() const
{
    const std::string s = attr(ATTR_IMAGE_WIDTH);
    if (s.empty())
        return -1;   // реф. дефолт при отсутствии/пустом
    KMeaStringUtil u;
    return u.ConvertStringToInt(s);
}

int KImageBlock::Heigth() const
{
    const std::string s = attr(ATTR_IMAGE_HEIGHT);
    if (s.empty())
        return -1;
    KMeaStringUtil u;
    return u.ConvertStringToInt(s);
}

std::string KImageBlock::GetAlign() const
{
    return attr(ATTR_ALIGN_H);   // как есть
}
