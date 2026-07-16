#include "report/KMeaXMLBase.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace {
// Литералы реф. (статические std::string-глобалы KMeaXMLBase.cpp).
const char *STR_NODE_ROOT      = "Root";
const char *STR_ATTR_CFGVERSION = "cfgver";
const char *STR_BACK_SUFFIX    = ".bak";

// реф. парсит БЕЗ parse_ws_pcdata → whitespace-only текст отбрасывается.
bool isBlankText(const QDomNode &n)
{
    return n.isText() && n.toText().data().trimmed().isEmpty();
}
} // namespace

int KMeaXMLBase::LoadXMLFile(const std::string &strFilePath, QDomDocument &doc)
{
    const QString path = QString::fromStdString(strFilePath);
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        // Реф.: отсутствующий файл — не отдельный код, а тот же -40 (load_file → false).
        qInfo() << "call load_file failed in KMeaXMLBase::LoadXMLFile! strFilePath=" << path;
        return -40;
    }
    // Реф. ошибку парсинга не детализирует (только факт) — err/line/col не прокидываем.
    if (!doc.setContent(&f)) {
        f.close();
        qInfo() << "call load_file failed in KMeaXMLBase::LoadXMLFile! strFilePath=" << path;
        return -40;
    }
    f.close();
    return 1;
}

int KMeaXMLBase::ParseXML(const std::string &strFilePath)
{
    QDomDocument doc;   // реф. — локальный документ, сразу разрушается
    const int ret = LoadXMLFile(strFilePath, doc);
    return ret == 1 ? 1 : -40;   // реф. здесь не логирует
}

bool KMeaXMLBase::IsFileExist(const std::string &strFilePath)
{
    return QFileInfo::exists(QString::fromStdString(strFilePath));
}

int KMeaXMLBase::ReplaceUserByLib(const std::string &strLibFile, const std::string &strUserFile)
{
    const QString lib  = QString::fromStdString(strLibFile);
    const QString user = QString::fromStdString(strUserFile);

    QDir().mkpath(QFileInfo(user).absolutePath());     // реф. fs::create_directories

    // Реф.: rename(user → user+".bak") — ошибка (например, файла ещё нет) игнорируется,
    // т.к. общий error_code проверяется ОДИН раз, уже после copy.
    const QString bak = user + STR_BACK_SUFFIX;
    QFile::remove(bak);
    QFile::rename(user, bak);

    if (!QFile::copy(lib, user)) {
        qWarning() << "Call fs::copy failed in KMeaXMLBase::ReplaceUserByLib! strLibFile="
                   << lib << ", strUserFile=" << user;
        return -23;
    }
    return 1;
}

int KMeaXMLBase::CheckVersion(const std::string &strFilePath) const
{
    QDomDocument doc;
    const int ret = LoadXMLFile(strFilePath, doc);
    if (ret != 1) {
        qWarning() << "Call LoadXMLFile failed in KMeaXMLBase::CheckVersion! ret=" << ret;
        return ret;
    }
    const int ver = GetModuleVersion();
    if (ver == -1)
        return ret;      // версия не объявлена → OK (реф. возвращает ret == 1)

    const QDomElement root = doc.documentElement();
    if (root.isNull() || root.tagName() != QLatin1String(STR_NODE_ROOT))
        return -46;
    if (root.attribute(STR_ATTR_CFGVERSION).toInt() != ver)
        return -46;
    return 1;
}

bool KMeaXMLBase::FindByName(const QDomElement &parent, const std::string &name,
                             QDomElement &out) const
{
    const QString want = QString::fromStdString(name);
    for (QDomNode n = parent.firstChild(); !n.isNull(); n = n.nextSibling()) {
        if (!n.isElement())
            continue;
        if (n.toElement().tagName() == want) {
            out = n.toElement();
            return true;
        }
    }
    return false;
}

bool KMeaXMLBase::FindByValue(const QDomElement &parent, const std::string &name,
                              const std::string &value, QDomElement &out) const
{
    const QString want = QString::fromStdString(name);
    const QString val  = QString::fromStdString(value);
    for (QDomNode n = parent.firstChild(); !n.isNull(); n = n.nextSibling()) {
        if (!n.isElement())
            continue;
        const QDomElement e = n.toElement();
        if (e.tagName() != want)
            continue;
        const QDomNode data = FindDataNode(e);   // реф. child_value() == текст первого pcdata
        if ((data.isNull() ? QString() : data.toText().data()) == val) {
            out = e;
            return true;
        }
    }
    return false;
}

bool KMeaXMLBase::FindByProperty(const QDomElement &parent, const std::string &name,
                                 const std::string &attr, const std::string &value,
                                 QDomElement &out) const
{
    const QString want = QString::fromStdString(name);
    const QString a    = QString::fromStdString(attr);
    const QString val  = QString::fromStdString(value);
    for (QDomNode n = parent.firstChild(); !n.isNull(); n = n.nextSibling()) {
        if (!n.isElement())
            continue;
        const QDomElement e = n.toElement();
        if (e.tagName() != want)
            continue;
        if (e.attribute(a) == val) {
            out = e;
            return true;
        }
    }
    return false;
}

QDomNode KMeaXMLBase::FindDataNode(const QDomElement &parent) const
{
    // Реф. берёт строго node_pcdata: CDATA не матчится, а whitespace-only узлов
    // у pugi просто нет (parse_ws_pcdata не выставлен) — эмулируем фильтром.
    for (QDomNode n = parent.firstChild(); !n.isNull(); n = n.nextSibling()) {
        if (n.isCDATASection())
            continue;
        if (n.isText() && !isBlankText(n))
            return n;
    }
    return QDomNode();
}

bool KMeaXMLBase::SetNodeValue(QDomElement node, const std::string &value) const
{
    if (node.isNull())
        return false;
    const QString v = QString::fromStdString(value);
    QDomNode data = FindDataNode(node);
    if (data.isNull()) {
        node.appendChild(node.ownerDocument().createTextNode(v));
        return true;
    }
    data.toText().setData(v);
    return true;
}

bool KMeaXMLBase::SetNodeProperty(QDomElement node, const std::string &attr,
                                  const std::string &value) const
{
    if (node.isNull())
        return false;
    node.setAttribute(QString::fromStdString(attr), QString::fromStdString(value));
    return true;
}
