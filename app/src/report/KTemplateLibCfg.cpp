#include "report/KTemplateLibCfg.h"
#include "report/KReportTemplateCommonDef.h"
#include "report/KTemplateCfg.h"
#include "sys/KEnvConfig.h"

#include <QDebug>
#include <QDomDocument>
#include <QDomElement>

#include <functional>
#include <set>
#include <vector>

namespace {
// Литералы реф. (STR_* — статические глобалы KTemplateLibCfg.cpp).
const char *STR_NODE_ROOT        = "root";
const char *STR_NODE_ITEM        = "Item";
const char *STR_ATTR_FORMAT      = "Format";
const char *STR_ATTR_REF         = "Ref";
const char *STR_TEMPLATE_CONTENT = "TemplateContent";
const char *SEP                  = "/";

// Каталоги/файлы ветки SubContent. Асимметрия "mainapp/" (RO есть, user нет) — как в реф.
const char *RO_SUBCONTENT_LIST = "mainapp/patient/report/template/SubContentList.xml";
const char *RO_SUBCONTENT_DIR  = "mainapp/patient/report/template/SubContent";
const char *RW_SUBCONTENT_DIR  = "patient/report/template/SubContent";
const char *RO_TEMPLATE_DIR    = "mainapp/patient/report/template/";
const char *RO_TEMPLATE_TYPES  = "mainapp/patient/report/template/TemplateTypes.xml";
} // namespace

int KTemplateLibCfg::Check(const std::string &strLibFile, const std::string &strUsrFile)
{
    (void)strLibFile;
    (void)strUsrFile;   // реф. игнорирует оба аргумента
    const std::string ro  = KEnvConfig::GetInstance().GetReadOnlyBaseDir() + SEP;
    const std::string usr = KEnvConfig::GetInstance().GetUsrDir() + SEP;

    // Порядок присвоения — как в реф.: 0x50, 0x70, 0x30, 0x90, 0xb0.
    m_strRoSubContentDir   = ro + RO_SUBCONTENT_DIR;
    m_strUserSubContentDir = usr + RW_SUBCONTENT_DIR;   // строится, но нигде не читается (реф.)
    m_strTemplateLibFile   = ro + RO_SUBCONTENT_LIST;
    m_strRoTemplateDir     = ro + RO_TEMPLATE_DIR;
    m_strTemplateTypesFile = ro + RO_TEMPLATE_TYPES;
    return 1;
}

int KTemplateLibCfg::LoadCache()
{
    KReportTemplateDataNew d;
    const int ret = LoadTemplateLib(m_strTemplateLibFile, d, std::string());
    if (ret == 1)
        m_data = std::move(d);
    else
        qWarning() << "LoadTemplateLib failed=! m_strTemplateLibFile="
                   << m_strTemplateLibFile.c_str();
    return ret;
}

int KTemplateLibCfg::LoadCacheGroup()
{
    const int ret = LoadTemplateLibs(m_strTemplateTypesFile);
    if (ret != 1)
        qWarning() << "LoadTemplateLib failed=! m_strTemplateLibFile="
                   << m_strTemplateTypesFile.c_str();
    return ret;
}

int KTemplateLibCfg::LoadTemplateLib(const std::string &strFile, KReportTemplateDataNew &data,
                                     std::string strUnused)
{
    (void)strUnused;   // реф. передаёт по значению и не использует

    QDomDocument doc;
    if (LoadXMLFile(strFile, doc) != 1) {
        qWarning() << "Load xml file failed! strFileName=" << strFile.c_str();
        return -40;
    }
    const QDomElement root = doc.documentElement();
    if (root.isNull() || root.tagName() != QLatin1String(STR_NODE_ROOT))
        return 0;   // реф.: нет корня → 0 (а НЕ -40)

    for (QDomNode n = root.firstChild(); !n.isNull(); n = n.nextSibling()) {
        if (!n.isElement())
            continue;
        const QDomElement e = n.toElement();
        if (e.tagName() != QLatin1String(STR_NODE_ITEM))
            continue;

        const QString format = e.attribute(STR_ATTR_FORMAT);
        const QString ref    = e.attribute(STR_ATTR_REF);
        // Реф.: пустой Format ЛИБО "TemplateContent".
        if (!format.isEmpty() && format != QLatin1String(STR_TEMPLATE_CONTENT))
            continue;

        const std::string path = m_strRoSubContentDir + SEP + ref.toUtf8().constData();
        KReportTemplateDataNew tmp;
        if (KTemplateCfg::ParseTemplateFile(path, tmp) != 1)
            continue;   // реф. молча пропускает битую запись (это НЕ ошибка)

        // Реф. создаёт out-список пустым и тут же выбрасывает результат.
        std::list<std::string> discarded;
        report_template::MergeData(data, tmp, discarded);
    }
    return 1;
}

int KTemplateLibCfg::LoadTemplateLibs(const std::string &strFile)
{
    QDomDocument doc;
    if (LoadXMLFile(strFile, doc) != 1) {
        qWarning() << "Load xml file failed! strFileName=" << strFile.c_str();
        return -40;
    }
    const QDomElement root = doc.documentElement();
    if (root.isNull() || root.tagName() != QLatin1String(STR_NODE_ROOT))
        return 0;

    for (QDomNode n = root.firstChild(); !n.isNull(); n = n.nextSibling()) {
        if (!n.isElement())
            continue;
        const QDomElement e = n.toElement();
        if (e.tagName() != QLatin1String(STR_NODE_ITEM))
            continue;
        if (e.attribute(STR_ATTR_FORMAT) != QLatin1String(STR_TEMPLATE_CONTENT))
            continue;

        const QString ref = e.attribute(STR_ATTR_REF);
        const std::string path = m_strRoTemplateDir + SEP + ref.toUtf8().constData();
        // Имя группы = basename Ref до ПЕРВОЙ точки (реф. strtok(Ref, ".")).
        const std::string name = ref.section('.', 0, 0).toUtf8().constData();

        KReportTemplateDataNew d;
        if (LoadTemplateLib(path, d, name) != 1) {
            // Реф.: цикл прерывается, но код возврата всё равно 1.
            qWarning() << "LoadTemplateLib failed=! m_strTemplateLibFile=" << path.c_str();
            break;
        }
        m_mapTemplateLibs[name] = std::move(d);   // существующий ключ перезаписывается
    }
    return 1;
}

KReportTemplateDataNew *KTemplateLibCfg::GetTemplateLib(const std::string &strName)
{
    const auto it = m_mapTemplateLibs.find(strName);
    if (it != m_mapTemplateLibs.end())
        return &it->second;
    return &m_data;   // реф.: при промахе — ссылка на m_data, НЕ nullptr, вставки нет
}

bool KTemplateLibCfg::RemoveNotUserItem(KReportTemplateDataNew &data)
{
    data.m_mapConfigs.clear();   // реф. — безусловно, первым делом

    // Оставляем только user-элементы; критерий — ровно m_bUserDefine.
    std::list<std::string> userNames;
    for (auto it = data.m_mapItemConfigs.begin(); it != data.m_mapItemConfigs.end();) {
        if (!it->second.m_bUserDefine) {
            it = data.m_mapItemConfigs.erase(it);
        } else {
            userNames.push_back(it->second.m_strName);   // реф. берёт поле, а НЕ ключ map
            ++it;
        }
    }

    // Множество допустимых ID: все префиксы длиной >= 2 + полный ID.
    std::set<std::string> keep;
    for (const std::string &name : userNames) {
        const std::vector<std::string> vec = report_template::RevertPathByID(
            name, report_template::STR_PATH_SEPARATOR);
        if (vec.size() <= 1)
            continue;   // префиксы длины 1 не включаются (реф.)
        for (size_t n = 2; n <= vec.size(); ++n) {
            std::vector<std::string> prefix(vec.begin(), vec.begin() + n);
            keep.insert(report_template::GenerateIDByPath(prefix,
                                                          report_template::STR_PATH_SEPARATOR));
        }
    }

    // Рекурсивно чистим дерево: нет ID в множестве → удалить вместе с поддеревом.
    std::function<void(std::list<KReportTemplateItem> &)> prune =
        [&](std::list<KReportTemplateItem> &lst) {
            for (auto it = lst.begin(); it != lst.end();) {
                if (keep.find(it->m_strID) == keep.end()) {
                    it = lst.erase(it);   // вместе со всем поддеревом
                } else {
                    prune(it->m_lstSubItems);
                    ++it;
                }
            }
        };
    prune(data.m_lstItems);
    return true;
}

int KTemplateLibCfg::UpdateTemplateLib(const std::string &strName,
                                       const KReportTemplateDataNew &data)
{
    (void)strName;   // реф. полностью игнорирует имя
    // Реф.: копия → фильтр → копия ВЫБРАСЫВАЕТСЯ; на диск не пишет.
    KReportTemplateDataNew copy = data;
    RemoveNotUserItem(copy);
    LoadCacheGroup();   // единственный устойчивый эффект — перечитывание с диска
    return 1;
}

int KTemplateLibCfg::UpdateTemplateLib(const KReportTemplateDataNew &data)
{
    KReportTemplateDataNew copy = data;
    RemoveNotUserItem(copy);
    LoadCache();        // отличие от перегрузки выше — ровно этот вызов
    return 1;
}
