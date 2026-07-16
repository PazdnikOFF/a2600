#include "report/KTemplateParamCfg.h"

#include <QDebug>
#include <QDomDocument>

namespace {
// Литералы реф. (файловые std::string-глобалы TU KTemplateParamCfg.cpp).
const char *STR_PATH_SEPARATOR  = "/";
const char *STR_REPORT_LIB_DIR  = "report/ReportTemplate/";   // = "report/" + "ReportTemplate" + "/"
const char *STR_PARAMFILE_NAME  = "ReportTemplateParam.xml";
const char *STR_NODE_ROOT       = "root";
const char *STR_NODE_ITEM       = "Item";
const char *STR_ATTR_NAME       = "Name";
const char *STR_ATTR_VALUE      = "Value";
} // namespace

int KTemplateParamCfg::Check(const std::string &strBaseDir, const std::string &strUnused)
{
    (void)strUnused;   // реф. затирает arg2 до первого чтения
    // Странность реф.: STR_REPORT_LIB_DIR уже оканчивается на '/', но разделитель
    // добавляется ещё раз → двойной слэш (Linux нормализует, безвредно). 1:1.
    m_strParamFile = strBaseDir + STR_REPORT_LIB_DIR + STR_PATH_SEPARATOR + STR_PARAMFILE_NAME;
    return 1;   // всегда
}

int KTemplateParamCfg::LoadCache()
{
    m_mapParams.clear();   // чистит именно вызывающий (реф.)
    return ParseParamFile(m_strParamFile, m_mapParams);
}

int KTemplateParamCfg::GetTemplateParam(const std::string &strGroup, const std::string &strKey,
                                        std::string &strValue)
{
    const auto g = m_mapParams.find(strGroup);
    if (g == m_mapParams.end())
        return 0;   // strValue не трогаем
    const auto k = g->second.find(strKey);
    if (k == g->second.end())
        return 0;
    strValue = k->second;
    return 1;
}

int KTemplateParamCfg::ParseParamGroup(const QDomElement &nodeGroup,
                                       std::map<std::string, std::string> &mapOut)
{
    for (QDomNode n = nodeGroup.firstChild(); !n.isNull(); n = n.nextSibling()) {
        if (!n.isElement())
            continue;
        const QDomElement e = n.toElement();
        if (e.tagName() != QLatin1String(STR_NODE_ITEM))
            continue;   // внутри группы — строго "Item"
        const QString name  = e.attribute(STR_ATTR_NAME);
        const QString value = e.attribute(STR_ATTR_VALUE);
        if (name.isEmpty() || value.isEmpty())
            continue;   // реф. требует, чтобы ОБА были непустые
        mapOut[name.toUtf8().constData()] = value.toUtf8().constData();
    }
    return 1;   // реф. — всегда 1, и вызывающий его игнорирует
}

int KTemplateParamCfg::ParseParamFile(const std::string &strFile,
                                      std::map<std::string, std::map<std::string, std::string>> &mapOut)
{
    QDomDocument doc;
    if (LoadXMLFile(strFile, doc) != 1) {
        qWarning() << "Load xml file failed! strFileName=" << strFile.c_str();
        return -40;
    }
    const QDomElement root = doc.documentElement();
    if (root.isNull() || root.tagName() != QLatin1String(STR_NODE_ROOT))
        return 0;   // нет корня → 0 (а не -40)

    for (QDomNode n = root.firstChild(); !n.isNull(); n = n.nextSibling()) {
        if (!n.isElement())
            continue;
        const QDomElement e = n.toElement();
        // Имя группы произвольно — фильтра по имени нет (реф.).
        const std::string group = e.tagName().toUtf8().constData();
        std::map<std::string, std::string> items;
        ParseParamGroup(e, items);   // возврат реф. игнорирует
        if (!group.empty() && !items.empty())
            mapOut[group] = std::move(items);   // пустые группы отбрасываются
    }
    return 1;
}
