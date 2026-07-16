#include "report/KTemplateCfg.h"
#include "sys/KEnvConfig.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringList>

#include <set>

namespace {
// Литералы реф. (STR_* — статические std::string-глобалы).
const char *STR_NODE_ROOT            = "root";
const char *STR_NODE_TEMPLATE_CONFIG = "TemplateConfig";
const char *STR_NODE_CONTENT         = "Content";
const char *STR_NODE_ITEM_CONFIG     = "ItemConfig";
const char *STR_NODE_ITEM            = "Item";
const char *STR_ATTR_NAME            = "Name";
const char *STR_ATTR_VALUE           = "Value";
const char *STR_ATTR_TITLE           = "Title";
const char *STR_ATTR_TYPE            = "Type";
const char *STR_ATTR_DATASRC         = "DataSrc";
const char *STR_ATTR_COLUMN          = "Column";
const char *STR_ATTR_SHOWTITLE       = "ShowTitle";
const char *STR_ATTR_USERDEFINE      = "UserDefine";
const char *STR_PATH_SEPARATOR       = "/";
const char *STR_RT_VALUE_1           = "1";
// Единственный дефолт реф.: пустой ShowTitle у этого типа → "1".
const char *STR_RT_TITLE_TABLE_BLOCK = "RT_TITLE_TABLE_BLOCK";

// Каталоги ветки FullTemplate. Асимметрия реф. (НЕ опечатка — два отдельных
// литерала): у RO-ветки есть "mainapp/", у user-ветки его нет.
const char *REPORT_TEMPLATE_RO = "mainapp/patient/report/template/FullTemplate/";
const char *REPORT_TEMPLATE_RW = "patient/report/template/FullTemplate/";

QString attr(const QDomElement &e, const char *name)
{
    return e.attribute(name);   // реф. as_string("") — отсутствующий атрибут → ""
}
} // namespace

KTemplateCfg::KTemplateCfg() = default;

int KTemplateCfg::Check(const std::string &strLibFile, const std::string &strUsrFile)
{
    (void)strLibFile;
    (void)strUsrFile;
    m_strRODir   = KEnvConfig::GetInstance().GetReadOnlyBaseDir() + STR_PATH_SEPARATOR
                 + REPORT_TEMPLATE_RO;
    m_strUserDir = KEnvConfig::GetInstance().GetUsrDir() + STR_PATH_SEPARATOR
                 + REPORT_TEMPLATE_RW;
    return 1;
}

int KTemplateCfg::LoadCache()
{
    return 1;   // реф. — 8-байтовая заглушка `mov w0,#1; ret`
}

void KTemplateCfg::GetUserConfigPath(std::string &outDir, std::string &outName) const
{
    outDir  = m_strUserDir;
    outName = "template";   // реф. STR_REPORT_TEMPLATE
}

std::string KTemplateCfg::GetTemplateFileName(const std::string &strName)
{
    return std::string("Template(") + strName + ").xml";
}

std::vector<std::string> KTemplateCfg::GetTemplateFiles(const std::string &strDir)
{
    std::vector<std::string> names;
    QDir dir(QString::fromStdString(strDir));
    if (!dir.exists())
        return names;

    // реф.: QDir::entryList + QRegularExpression("Template\\((.+)\\).xml") → captured(1)
    static const QRegularExpression re(QStringLiteral("Template\\((.+)\\).xml"));
    const QStringList entries = dir.entryList(QDir::Files);
    for (const QString &e : entries) {
        const QRegularExpressionMatch m = re.match(e);
        if (m.hasMatch())
            names.push_back(m.captured(1).toUtf8().constData());
    }
    return names;
}

std::vector<std::string> KTemplateCfg::GetLibTemplateFiles() const
{
    return GetTemplateFiles(m_strRODir);
}

std::vector<std::string> KTemplateCfg::GetUserTemplateFiles() const
{
    return GetTemplateFiles(m_strUserDir);
}

// ————— Разбор —————

void KTemplateCfg::ParseTemplateConfig(const QDomElement &node,
                                       std::map<std::string, std::string> &out)
{
    for (QDomNode n = node.firstChild(); !n.isNull(); n = n.nextSibling()) {
        if (!n.isElement())
            continue;
        const QDomElement e = n.toElement();
        if (e.tagName() != QLatin1String(STR_NODE_ITEM))
            continue;
        out[attr(e, STR_ATTR_NAME).toUtf8().constData()] =
            attr(e, STR_ATTR_VALUE).toUtf8().constData();
    }
}

void KTemplateCfg::ParseTemplateContent(const QDomElement &node, const std::string &strParentPath,
                                        std::list<KReportTemplateItem> &out)
{
    std::set<std::string> seen;   // реф. дедупит Name среди сиблингов
    for (QDomNode n = node.firstChild(); !n.isNull(); n = n.nextSibling()) {
        if (!n.isElement())
            continue;
        const QDomElement e = n.toElement();
        if (e.tagName() != QLatin1String(STR_NODE_ITEM))
            continue;

        KReportTemplateItem item;
        item.m_strName      = attr(e, STR_ATTR_NAME).toUtf8().constData();
        item.m_strTitle     = attr(e, STR_ATTR_TITLE).toUtf8().constData();
        item.m_strType      = attr(e, STR_ATTR_TYPE).toUtf8().constData();
        item.m_strDataSrc   = attr(e, STR_ATTR_DATASRC).toUtf8().constData();
        item.m_strColumn    = attr(e, STR_ATTR_COLUMN).toUtf8().constData();
        item.m_strShowTitle = attr(e, STR_ATTR_SHOWTITLE).toUtf8().constData();
        // Единственный дефолт реф.
        if (item.m_strShowTitle.empty() && item.m_strType == STR_RT_TITLE_TABLE_BLOCK)
            item.m_strShowTitle = STR_RT_VALUE_1;

        item.m_strID = strParentPath + STR_PATH_SEPARATOR + item.m_strName;

        if (!seen.insert(item.m_strName).second) {
            qWarning() << "Repeat template config item" << item.m_strID.c_str();
            continue;
        }

        ParseTemplateContent(e, item.m_strID, item.m_lstSubItems);   // рекурсия
        out.push_back(item);
    }
}

void KTemplateCfg::ParseTemplateItemConfig(const QDomElement &node,
                                           std::map<std::string, KReportTemplateItemConfig> &out)
{
    for (QDomNode n = node.firstChild(); !n.isNull(); n = n.nextSibling()) {
        if (!n.isElement())
            continue;
        const QDomElement e = n.toElement();
        if (e.tagName() != QLatin1String(STR_NODE_ITEM))
            continue;

        KReportTemplateItemConfig cfg;
        cfg.m_strName = attr(e, STR_ATTR_NAME).toUtf8().constData();
        // Реф. НЕ раскладывает атрибуты по типизированным полям — все пары as-is
        // в generic-map (включая сам Name). Пустое имя ИЛИ пустое значение — пропуск.
        const QDomNamedNodeMap attrs = e.attributes();
        for (int i = 0; i < attrs.count(); ++i) {
            const QDomAttr a = attrs.item(i).toAttr();
            if (a.name().isEmpty() || a.value().isEmpty())
                continue;
            cfg.m_mapAttrs[a.name().toUtf8().constData()] = a.value().toUtf8().constData();
        }
        cfg.m_bUserDefine = attr(e, STR_ATTR_USERDEFINE) == QLatin1String(STR_RT_VALUE_1);

        out[cfg.m_strName] = cfg;
    }
}

int KTemplateCfg::ParseTemplateFile(const std::string &strFile, KReportTemplateDataNew &out)
{
    QDomDocument doc;
    const int ret = LoadXMLFile(strFile, doc);
    if (ret != 1) {
        qWarning() << "Load xml file failed! strTemplateFile=" << strFile.c_str();
        return ret;
    }
    const QDomElement root = doc.documentElement();
    if (root.isNull() || root.tagName() != QLatin1String(STR_NODE_ROOT))
        return -40;

    for (QDomNode n = root.firstChild(); !n.isNull(); n = n.nextSibling()) {
        if (!n.isElement())
            continue;
        const QDomElement e = n.toElement();
        if (e.tagName() == QLatin1String(STR_NODE_TEMPLATE_CONFIG))
            ParseTemplateConfig(e, out.m_mapConfigs);
        else if (e.tagName() == QLatin1String(STR_NODE_CONTENT))
            ParseTemplateContent(e, std::string(), out.m_lstItems);
        else if (e.tagName() == QLatin1String(STR_NODE_ITEM_CONFIG))
            ParseTemplateItemConfig(e, out.m_mapItemConfigs);
    }
    return 1;
}

// ————— Запись —————

void KTemplateCfg::SaveTemplateConfig(QDomElement &node,
                                      const std::map<std::string, std::string> &in)
{
    QDomDocument doc = node.ownerDocument();
    for (const auto &kv : in) {
        QDomElement e = doc.createElement(STR_NODE_ITEM);
        e.setAttribute(STR_ATTR_NAME, QString::fromStdString(kv.first));
        e.setAttribute(STR_ATTR_VALUE, QString::fromStdString(kv.second));
        node.appendChild(e);
    }
}

void KTemplateCfg::SaveTemplateContent(QDomElement &node,
                                       const std::list<KReportTemplateItem> &in)
{
    QDomDocument doc = node.ownerDocument();
    for (const KReportTemplateItem &item : in) {
        QDomElement e = doc.createElement(STR_NODE_ITEM);
        e.setAttribute(STR_ATTR_NAME, QString::fromStdString(item.m_strName));
        // Пустые атрибуты не пишем — реф. читает их как "" (симметрия чтения/записи).
        if (!item.m_strTitle.empty())
            e.setAttribute(STR_ATTR_TITLE, QString::fromStdString(item.m_strTitle));
        if (!item.m_strType.empty())
            e.setAttribute(STR_ATTR_TYPE, QString::fromStdString(item.m_strType));
        if (!item.m_strShowTitle.empty())
            e.setAttribute(STR_ATTR_SHOWTITLE, QString::fromStdString(item.m_strShowTitle));
        if (!item.m_strColumn.empty())
            e.setAttribute(STR_ATTR_COLUMN, QString::fromStdString(item.m_strColumn));
        if (!item.m_strDataSrc.empty())
            e.setAttribute(STR_ATTR_DATASRC, QString::fromStdString(item.m_strDataSrc));
        SaveTemplateContent(e, item.m_lstSubItems);   // рекурсия
        node.appendChild(e);
    }
}

void KTemplateCfg::SaveTemplateItemConfig(QDomElement &node,
                                          const std::map<std::string, KReportTemplateItemConfig> &in)
{
    QDomDocument doc = node.ownerDocument();
    for (const auto &kv : in) {
        QDomElement e = doc.createElement(STR_NODE_ITEM);
        // m_mapAttrs уже содержит Name — пишем всё как есть.
        for (const auto &a : kv.second.m_mapAttrs)
            e.setAttribute(QString::fromStdString(a.first), QString::fromStdString(a.second));
        if (!e.hasAttribute(STR_ATTR_NAME))
            e.setAttribute(STR_ATTR_NAME, QString::fromStdString(kv.second.m_strName));
        node.appendChild(e);
    }
}

int KTemplateCfg::SaveTemplateFile(const std::string &strFile, const KReportTemplateDataNew &data)
{
    QDomDocument doc;
    doc.appendChild(doc.createProcessingInstruction(
        QStringLiteral("xml"), QStringLiteral("version=\"1.0\"")));
    QDomElement root = doc.createElement(STR_NODE_ROOT);
    doc.appendChild(root);

    QDomElement tcfg = doc.createElement(STR_NODE_TEMPLATE_CONFIG);
    root.appendChild(tcfg);
    SaveTemplateConfig(tcfg, data.m_mapConfigs);

    QDomElement content = doc.createElement(STR_NODE_CONTENT);
    root.appendChild(content);
    SaveTemplateContent(content, data.m_lstItems);

    QDomElement icfg = doc.createElement(STR_NODE_ITEM_CONFIG);
    root.appendChild(icfg);
    SaveTemplateItemConfig(icfg, data.m_mapItemConfigs);

    const QString path = QString::fromStdString(strFile);
    QDir().mkpath(QFileInfo(path).absolutePath());   // реф. boost::filesystem
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning() << "call save_file failed in KTemplateCfg::SaveTemplateFile! strFile=" << path;
        return -40;
    }
    f.write(doc.toByteArray(4));
    f.close();
    return 1;
}

// ————— Кэш —————

int KTemplateCfg::GetTemplateCfg(const std::string &strName, KReportTemplateDataNew &out)
{
    const std::string file = GetTemplateFileName(strName);

    const auto it = m_mapCache.find(file);   // реф. ключ кэша — ИМЯ ФАЙЛА, не strName
    if (it != m_mapCache.end()) {
        out = it->second;
        return 1;
    }

    // Промах: сначала user-ветка, при неудаче — фолбэк на RO-ветку (реф.).
    int ret = ParseTemplateFile(m_strUserDir + file, out);
    if (ret != 1)
        ret = ParseTemplateFile(m_strRODir + file, out);
    if (ret == 1)
        m_mapCache[file] = out;
    return ret;
}

int KTemplateCfg::GetSubTemplateData(const std::string &strName, KReportTemplateDataNew &out)
{
    // Реф. только копирует из кэша, на диск не ходит.
    const auto it = m_mapCache.find(GetTemplateFileName(strName));
    if (it == m_mapCache.end())
        return -40;
    out = it->second;
    return 1;
}

int KTemplateCfg::UpdateTemplateCfg(const std::string &strName, const KReportTemplateDataNew &data)
{
    const std::string file = GetTemplateFileName(strName);
    const int ret = SaveTemplateFile(m_strUserDir + file, data);
    if (ret != 1)
        return ret;
    m_mapCache[file] = data;
    return 1;
}

int KTemplateCfg::DeleteTemplateCfg(const std::string &strName)
{
    const std::string file = GetTemplateFileName(strName);
    m_mapCache.erase(file);
    QFile::remove(QString::fromStdString(m_strUserDir + file));   // реф. fs::remove
    return 1;
}
