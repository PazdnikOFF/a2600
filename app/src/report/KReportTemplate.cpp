#include "report/KReportTemplate.h"
#include "report/KTemplateCfg.h"
#include "report/KTemplateLibCfg.h"
#include "report/KRTDataSourceDemo.h"
#include "report/KRTDataSourceReal.h"
#include "sys/KEnvConfig.h"
#include "sys/KSystem.h"

#include <QDir>
#include <QFile>
#include <QDomDocument>
#include <QHash>

#include <cassert>

QString ReportItem::dataSource() const
{
    const int c = dataSrc.indexOf(',');
    return c < 0 ? QString() : dataSrc.left(c);
}

QString ReportItem::dataField() const
{
    const int c = dataSrc.indexOf(',');
    return c < 0 ? QString() : dataSrc.mid(c + 1);
}

KReportTemplateManager::KReportTemplateManager(const QString &reportRoot)
    : reportRoot_(reportRoot)
{
    if (reportRoot_.isEmpty())
        reportRoot_ = QDir(KSystem::SystemPath())
            .absoluteFilePath("presetdata/syspreset/mainapp/patient/report");
}

QStringList KReportTemplateManager::TemplateNames() const
{
    QStringList names;
    QFile f(QDir(reportRoot_).absoluteFilePath("config/TempletInfo.xml"));
    if (!f.open(QIODevice::ReadOnly))
        return names;
    QDomDocument doc;
    if (!doc.setContent(&f))
        return names;
    QDomElement root = doc.documentElement();
    for (QDomElement e = root.firstChildElement("Templet"); !e.isNull();
         e = e.nextSiblingElement("Templet"))
        names << e.attribute("name");
    return names;
}

// Рекурсивный разбор <Item> в ReportItem.
static ReportItem parseItem(const QDomElement &e)
{
    ReportItem it;
    it.name      = e.attribute("Name");
    it.title     = e.attribute("Title");
    it.type      = e.attribute("Type");
    it.dataSrc   = e.attribute("DataSrc");
    it.column    = e.attribute("Column", "1").toInt();
    it.showTitle = e.attribute("ShowTitle", "0").toInt() != 0;
    for (QDomElement c = e.firstChildElement("Item"); !c.isNull();
         c = c.nextSiblingElement("Item"))
        it.children.append(parseItem(c));
    return it;
}

// Применить <ItemConfig> к дереву по пути элемента ("/A/B/C").
static void applyConfig(ReportItem &it, const QString &parentPath,
                        const QHash<QString, QDomElement> &cfg)
{
    const QString path = parentPath + "/" + it.name;
    auto found = cfg.find(path);
    if (found != cfg.end()) {
        const QDomElement &c = found.value();
        it.imageWidth = c.attribute("ImageWidth", "0").toInt();
        it.alignH     = c.attribute("AlignH");
        it.fontType   = c.attribute("FontType");
        it.section    = c.attribute("Section");
        it.lineHeight = c.attribute("LineHeight1", "0").toInt();
    }
    for (ReportItem &ch : it.children)
        applyConfig(ch, path, cfg);
}

QVector<ReportItem> KReportTemplateManager::loadSubContent(const QString &fileName) const
{
    QVector<ReportItem> items;
    QFile f(QDir(reportRoot_).absoluteFilePath("template/SubContent/" + fileName));
    if (!f.open(QIODevice::ReadOnly))
        return items;
    QDomDocument doc;
    if (!doc.setContent(&f))
        return items;
    QDomElement rootEl = doc.documentElement();
    // <root><Content><Item>…</Item></Content>…
    QDomElement content = rootEl.firstChildElement("Content");
    for (QDomElement e = content.firstChildElement("Item"); !e.isNull();
         e = e.nextSiblingElement("Item"))
        items.append(parseItem(e));

    // <ItemConfig><Item Name="/путь" ImageWidth=.. AlignH=.. FontType=.. Section=../>
    QHash<QString, QDomElement> cfg;
    QDomElement icfg = rootEl.firstChildElement("ItemConfig");
    for (QDomElement e = icfg.firstChildElement("Item"); !e.isNull();
         e = e.nextSiblingElement("Item"))
        cfg.insert(e.attribute("Name"), e);
    for (ReportItem &it : items)
        applyConfig(it, QString(), cfg);
    return items;
}

QVector<ReportItem> KReportTemplateManager::LoadTemplate(const QString &name) const
{
    QVector<ReportItem> result;
    // ReportTemplateNP-<L>.xml — список <Item Ref="<SubContent>.xml"/>.
    const QString tplFile = QString("template/ReportTemplateNP-%1.xml")
        .arg(name.startsWith("NP-") ? name.mid(3) : name);
    QFile f(QDir(reportRoot_).absoluteFilePath(tplFile));
    if (!f.open(QIODevice::ReadOnly))
        return result;
    QDomDocument doc;
    if (!doc.setContent(&f))
        return result;
    QDomElement root = doc.documentElement();
    for (QDomElement e = root.firstChildElement("Item"); !e.isNull();
         e = e.nextSiblingElement("Item")) {
        const QString ref = e.attribute("Ref");
        if (!ref.isEmpty())
            result += loadSubContent(ref);
    }
    return result;
}

// ============================================================================
// Фейтфул-API референса KReportTemplateManager (синглтон + InitModule/части).
// Пути-литералы сверены из дизасма (анонимные .bss-строки TU):
//   REPORT_TEMPLATE_RO = "mainapp/patient/report/"   REPORT_TEMPLATE_RW = "patient/report/"
//   REPORT_RW_BACKUP   = "mainapp/patient/report/rw/report"  (заводское дерево для провизии)
// ============================================================================

KReportTemplateManager::~KReportTemplateManager()
{
    UninitModule();
}

KReportTemplateManager *KReportTemplateManager::GetInstance()
{
    // реф. — heap shared_ptr + std::call_once; Meyers-static эквивалентен и потокобезопасен.
    // GetInstance НЕ инициализирует модуль (реф.).
    static KReportTemplateManager s_instance;
    return &s_instance;
}

void KReportTemplateManager::InitTempletsInfos()
{
    // Каталог шаблонов — RO-дерево прошивки (config/TempletInfo.xml).
    const QString roReport =
        QString::fromStdString(KEnvConfig::GetInstance().GetReadOnlyBaseDir())
        + "/mainapp/patient/report";
    KSysReportTempletCfg &cfg = KSysReportTempletCfg::GetInstance();
    cfg.SetReportRoot(roReport);
    cfg.Reload();
    m_vecTempletInfos = cfg.TempletInfos();
}

void KReportTemplateManager::InitTempletLibInfos()
{
    // Reload уже выполнен в InitTempletsInfos — забираем библиотечный каталог.
    m_vecTempletLibInfos = KSysReportTempletCfg::GetInstance().TempletLibInfos();
}

void KReportTemplateManager::GetTempletLibName(const std::string &templName,
                                               std::string &out) const
{
    // Реф. @0x5999d0. ПРОМАХ не трогает out — поэтому НЕ очищаем его здесь.
    for (const KTempletBaseInfo &libInfo : m_vecTempletLibInfos) {
        // depts держится отсортированным по имени → порядок как у реф. map<string,bool>.
        for (const KTempletDept &dept : libInfo.depts) {
            const std::string key = dept.name.toStdString();
            // ⚠️ Реф. substr(3) без guard роняет приложение на key длиной <3; у нас —
            // пропуск (в поставке депты всегда "KW_*", ветка не срабатывает).
            if (key.size() < 3)
                continue;
            if (key.substr(3) == templName) {
                out = libInfo.TempletName().toStdString();  // имя библиотеки
                return;                                     // первое совпадение — выход
            }
        }
    }
    // Промах: out оставлен как есть (реф. голый return).
}

int KReportTemplateManager::InitModule()
{
    if (m_bInited)
        return 1;

    const std::string roBase = KEnvConfig::GetInstance().GetReadOnlyBaseDir();
    const std::string usrBase = KEnvConfig::GetInstance().GetUsrDir();
    const QString ro = QString::fromStdString(roBase);
    const QString usr = QString::fromStdString(usrBase);
    const QString usrReport = usr + "/patient/report/";
    const QString provSrc = ro + "/mainapp/patient/report/rw/report";

    // Первичная провизия: заводское RO-дерево → userpreset (реф.; userpreset gitignored).
    if (!QDir(usrReport).exists()) {
        KSystem::CopyDirectoryFiles(provSrc, usrReport, true);
    } else {
        const bool need = !QFile::exists(usrReport + "config/TempletInfo.xml")
            || !QFile::exists(usrReport + "config/TempletLibInfo.xml")
            || !QFile::exists(usrReport + "config/ReportTemplateConfig.xml");
        if (need)
            KSystem::CopyDirectoryFiles(provSrc, usrReport, true);
    }

    InitTempletsInfos();
    InitTempletLibInfos();

    m_pTemplateCfg = new KTemplateCfg();
    m_pTemplateLibCfg = new KTemplateLibCfg();
    m_pDemoDataSource = new KRTDataSourceDemo();
    m_pDataSourceReal = new KRTDataSourceReal();

    // Аргументы Check реф. игнорирует (строит пути из KEnvConfig) — передаём faithful-значения.
    const std::string roArg = roBase + "/mainapp/patient/report/";
    const std::string usrArg = usrBase + "/patient/report/";

    m_pTemplateCfg->Check(roArg, usrArg);
    m_pTemplateCfg->LoadCache();

    m_pTemplateLibCfg->Check(roArg, usrArg);
    m_pTemplateLibCfg->LoadCache();
    m_pTemplateLibCfg->LoadCacheGroup();
    m_pTemplateLibCfg->UpdateTemplateLib(m_pTemplateLibCfg->Data());

    m_bInited = true;
    return 1;
}

int KReportTemplateManager::UninitModule()
{
    delete m_pTemplateCfg;    m_pTemplateCfg = nullptr;
    delete m_pTemplateLibCfg; m_pTemplateLibCfg = nullptr;
    delete m_pDemoDataSource; m_pDemoDataSource = nullptr;
    delete m_pDataSourceReal; m_pDataSourceReal = nullptr;
    m_bInited = false;   // реф. — векторы НЕ чистит (только dtor)
    return 1;
}

KTemplateCfg *KReportTemplateManager::GetTemplateCfg() const
{
    assert(m_pTemplateCfg);
    return m_pTemplateCfg;
}

KTemplateLibCfg *KReportTemplateManager::GetTemplateLibCfg() const
{
    assert(m_pTemplateLibCfg);
    return m_pTemplateLibCfg;
}

KRTDataSourceDemo *KReportTemplateManager::GetDemoDataSource() const
{
    assert(m_pDemoDataSource);
    return m_pDemoDataSource;
}

KRTDataSourceReal *KReportTemplateManager::GetDataSourceReal() const
{
    return m_pDataSourceReal;   // реф. — БЕЗ assert (null-safe)
}
