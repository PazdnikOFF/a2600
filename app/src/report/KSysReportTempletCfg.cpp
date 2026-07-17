#include "report/KSysReportTempletCfg.h"
#include "report/KReportTemplate.h"
#include "report/KTemplateCfg.h"
#include "report/KTemplateLibCfg.h"
#include "sys/KEnvConfig.h"
#include "sys/KSystem.h"

#include <algorithm>

#include <QDir>
#include <QFile>
#include <QDomDocument>
#include <QFileInfo>
#include <QTextStream>

// --- мутаторы реф.-семантики (в реф. depts — std::map<string,bool>: уникальность + сортировка) ---

bool KTempletBaseInfo::IsDefault(const QString &dept) const
{
    for (const KTempletDept &d : depts)
        if (d.name == dept)
            return d.isDefault;
    return false;
}

void KTempletBaseInfo::SetDefault(const QString &dept, bool v)
{
    for (KTempletDept &d : depts)
        if (d.name == dept) {
            d.isDefault = v;
            return;
        }
}

void KTempletBaseInfo::AddDept(const QString &dept, bool isDefault)
{
    for (KTempletDept &d : depts)          // map-семантика: существующий ключ → перезапись флага
        if (d.name == dept) {
            d.isDefault = isDefault;
            return;
        }
    KTempletDept nd; nd.name = dept; nd.isDefault = isDefault;
    // вставка с сохранением сортировки по имени (обход map в реф. — по возрастанию ключа)
    int i = 0;
    while (i < depts.size() && depts[i].name < dept)
        ++i;
    depts.insert(i, nd);
}

void KTempletBaseInfo::DeleteDept(const QString &dept)
{
    for (int i = 0; i < depts.size(); ++i)
        if (depts[i].name == dept) {
            depts.remove(i);
            return;
        }
}

QStringList KTempletBaseInfo::deptNames() const
{
    QStringList out;
    for (const KTempletDept &d : depts)
        out << d.name;
    return out;
}

bool KTempletBaseInfo::hasDept(const QString &dept) const
{
    for (const KTempletDept &d : depts)
        if (d.name == dept)
            return true;
    return false;
}

KSysReportTempletCfg &KSysReportTempletCfg::GetInstance()
{
    static KSysReportTempletCfg inst;
    return inst;
}

QString KSysReportTempletCfg::ReportRoot() const
{
    if (!reportRoot_.isEmpty())
        return reportRoot_;
    // Как у KReportTemplateManager: .../mainapp/patient/report.
    return QDir(KSystem::SystemPath())
        .absoluteFilePath("presetdata/syspreset/mainapp/patient/report");
}

void KSysReportTempletCfg::ensureLoaded() const
{
    if (!loaded_)
        const_cast<KSysReportTempletCfg *>(this)->Reload();
}

bool KSysReportTempletCfg::parseTempletFile(const QString &file,
                                            QVector<KTempletBaseInfo> &out) const
{
    // Имя ОТНОСИТЕЛЬНО ReportRoot()/config/ (старый read-путь).
    return parseTempletFileAbs(QDir(ReportRoot()).absoluteFilePath("config/" + file), out);
}

bool KSysReportTempletCfg::parseTempletFileAbs(const QString &absFile,
                                               QVector<KTempletBaseInfo> &out) const
{
    // АБСОЛЮТНЫЙ путь (реф.-методы ходят по KEnvConfig RO/RW).
    out.clear();
    QFile f(absFile);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    QDomDocument doc;
    if (!doc.setContent(&f))
        return false;

    QDomElement root = doc.documentElement();
    for (QDomElement e = root.firstChildElement("Templet"); !e.isNull();
         e = e.nextSiblingElement("Templet")) {
        KTempletBaseInfo info;
        info.name = e.attribute("name");
        info.modifyDate = e.attribute("modifydate");
        info.factory = e.attribute("factory") == "1";
        for (QDomElement d = e.firstChildElement("Dept"); !d.isNull();
             d = d.nextSiblingElement("Dept")) {
            KTempletDept dept;
            dept.name = d.attribute("name");
            dept.isDefault = d.attribute("default") == "1";
            info.depts << dept;
        }
        out << info;
    }
    return true;
}

bool KSysReportTempletCfg::Reload()
{
    loaded_ = true;
    // Библиотека шаблонов (TempletLibInfo.xml) — не критична для успеха.
    parseTempletFile("TempletLibInfo.xml", libInfos_);
    // Основной каталог (TempletInfo.xml).
    return parseTempletFile("TempletInfo.xml", infos_);
}

QStringList KSysReportTempletCfg::TempletNames() const
{
    ensureLoaded();
    QStringList out;
    for (const KTempletBaseInfo &i : infos_)
        out << i.name;
    return out;
}

KTempletBaseInfo KSysReportTempletCfg::GetTemplateInfoByName(const QString &name,
                                                             bool *found) const
{
    ensureLoaded();
    for (const KTempletBaseInfo &i : infos_)
        if (i.name == name) {
            if (found) *found = true;
            return i;
        }
    if (found) *found = false;
    return KTempletBaseInfo();
}

QStringList KSysReportTempletCfg::GetDeptsOfTemplate(const QString &name) const
{
    return GetTemplateInfoByName(name).deptNames();
}

QStringList KSysReportTempletCfg::GetTempletNamesByDept(const QString &dept) const
{
    ensureLoaded();
    QStringList out;
    for (const KTempletBaseInfo &i : infos_)
        if (i.hasDept(dept))
            out << i.name;
    return out;
}

QString KSysReportTempletCfg::GetDefaultTempletByDept(const QString &dept) const
{
    ensureLoaded();
    for (const KTempletBaseInfo &i : infos_)
        for (const KTempletDept &d : i.depts)
            if (d.name == dept && d.isDefault)
                return i.name;
    return QString();
}

QStringList KSysReportTempletCfg::TempletLibNames() const
{
    ensureLoaded();
    QStringList out;
    for (const KTempletBaseInfo &i : libInfos_)
        out << i.name;
    return out;
}

KTempletBaseInfo KSysReportTempletCfg::GetLibTemplateInfoByName(const QString &name,
                                                                bool *found) const
{
    ensureLoaded();
    for (const KTempletBaseInfo &i : libInfos_)
        if (i.name == name) {
            if (found) *found = true;
            return i;
        }
    if (found) *found = false;
    return KTempletBaseInfo();
}

// ============================================================================
// Реф.-методы загрузки/записи (сверено дизасмом). Пути — как в реф., через KEnvConfig:
//   RO = GetReadOnlyBaseDir() + "mainapp/patient/report/config/TempletInfo.xml"
//   RW = GetUsrDir()          + "patient/report/config/TempletInfo.xml"
// RO/RW — РАЗНЫЕ префиксы, не суффикс одного корня.
// ============================================================================

QString KSysReportTempletCfg::roTempletInfoFile()
{
    return QDir(QString::fromStdString(KEnvConfig::GetInstance().GetReadOnlyBaseDir()))
        .absoluteFilePath("mainapp/patient/report/config/TempletInfo.xml");
}

QString KSysReportTempletCfg::rwTempletInfoFile()
{
    return QDir(QString::fromStdString(KEnvConfig::GetInstance().GetUsrDir()))
        .absoluteFilePath("patient/report/config/TempletInfo.xml");
}

bool KSysReportTempletCfg::saveTempletFile(const QString &file, const QVector<KTempletBaseInfo> &infos)
{
    // Схема реф. (KTempletInfoFileHandler::Save/AddTempletNode): корень "Root", по <Templet>
    // на инфо (ВСЕ, без фильтра, в порядке вектора), внутри <Dept> по возрастанию имени.
    QDomDocument doc;
    QDomElement root = doc.createElement("Root");
    doc.appendChild(root);
    for (const KTempletBaseInfo &info : infos) {
        QDomElement t = doc.createElement("Templet");
        t.setAttribute("name", info.name);                       // порядок атрибутов как в реф.
        t.setAttribute("modifydate", info.modifyDate);
        t.setAttribute("factory", info.factory ? "1" : "0");     // РОВНО "1"/"0" (не true/false)
        QVector<KTempletDept> depts = info.depts;                // реф. depts — std::map → сорт по имени
        std::sort(depts.begin(), depts.end(),
                  [](const KTempletDept &a, const KTempletDept &b) { return a.name < b.name; });
        for (const KTempletDept &d : depts) {
            QDomElement e = doc.createElement("Dept");
            e.setAttribute("name", d.name);
            e.setAttribute("default", d.isDefault ? "1" : "0");
            t.appendChild(e);
        }
        root.appendChild(t);
    }
    QDir().mkpath(QFileInfo(file).absolutePath());
    QFile f(file);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;   // реф. — LogPrintfEx "Save templetInfo file failed …"
    QTextStream ts(&f);
    doc.save(ts, 1);    // реф. pugixml: format_indent, отступ таб
    f.close();
    return true;
}

void KSysReportTempletCfg::LoadDefault()
{
    // 1) снести per-template user-cfg каждого известного шаблона
    KTemplateCfg *cfg = KReportTemplateManager::GetInstance()->GetTemplateCfg();
    for (const KTempletBaseInfo &info : infos_)
        if (cfg)
            cfg->DeleteTemplateCfg(info.name.toStdString());
    // 2) сброс либ-кэша дефолтным (пустым) data — реф. UpdateTemplateLib(KReportTemplateDataNew())
    if (KTemplateLibCfg *lib = KReportTemplateManager::GetInstance()->GetTemplateLibCfg())
        lib->UpdateTemplateLib(KReportTemplateDataNew());
    // 3) перечитать RO-каталог (только infos_; libInfos_ реф. НЕ трогает)
    parseTempletFileAbs(roTempletInfoFile(), infos_);
    loaded_ = true;
    // реф. НЕ пишет TempletInfo.xml здесь — user-файл остаётся прежним до явного Save.
}

void KSysReportTempletCfg::LoadUserXML()
{
    // реф.: без CheckFile и без RO-фолбэка — просто читает USER-файл в infos_.
    parseTempletFileAbs(rwTempletInfoFile(), infos_);
    loaded_ = true;
}

void KSysReportTempletCfg::GetTemplateByName(const std::string &name, KReportTemplateDataNew &out)
{
    // реф. — делегирование в KTemplateCfg::GetTemplateCfg (Template(<имя>).xml, user→RO + кэш).
    if (KTemplateCfg *cfg = KReportTemplateManager::GetInstance()->GetTemplateCfg())
        cfg->GetTemplateCfg(name, out);   // реф. возврат игнорирует (только printf успех/провал)
}

void KSysReportTempletCfg::SaveTempletInfos(const QVector<KTempletBaseInfo> &infos)
{
    infos_ = infos;                       // реф. сперва кэширует аргумент в член…
    loaded_ = true;
    saveTempletFile(rwTempletInfoFile(), infos_);   // …затем сериализует ЕГО в USER-путь
}

void KSysReportTempletCfg::SaveTemplateCfg(const std::map<std::string, KReportTemplateDataNew> &cfgs,
                                           const std::vector<std::string> &delList)
{
    KTemplateCfg *cfg = KReportTemplateManager::GetInstance()->GetTemplateCfg();
    if (!cfg)
        return;
    // 1) УДАЛЕНИЯ ИДУТ ПЕРВЫМИ (реф. порядок, хотя delList — второй аргумент).
    for (const std::string &name : delList)
        cfg->DeleteTemplateCfg(name);
    // 2) апдейты; ФИЛЬТР: нетронутый заводской (factory && modifydate=="factory") — ПРОПУСК.
    for (const auto &kv : cfgs) {
        bool found = false;
        const KTempletBaseInfo info =
            GetTemplateInfoByName(QString::fromStdString(kv.first), &found);
        if (found && info.factory && info.modifyDate == "factory")
            continue;
        cfg->UpdateTemplateCfg(kv.first, kv.second);
    }
}
