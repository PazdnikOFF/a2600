#include "report/KSysReportTempletCfg.h"
#include "sys/KSystem.h"

#include <QDir>
#include <QFile>
#include <QDomDocument>

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

bool KSysReportTempletCfg::Reload()
{
    infos_.clear();
    loaded_ = true;
    QFile f(QDir(ReportRoot()).absoluteFilePath("config/TempletInfo.xml"));
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
        infos_ << info;
    }
    return true;
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
