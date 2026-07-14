#include "report/KReportTemplate.h"
#include "sys/KSystem.h"

#include <QDir>
#include <QFile>
#include <QDomDocument>

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

QVector<ReportItem> KReportTemplateManager::loadSubContent(const QString &fileName) const
{
    QVector<ReportItem> items;
    QFile f(QDir(reportRoot_).absoluteFilePath("template/SubContent/" + fileName));
    if (!f.open(QIODevice::ReadOnly))
        return items;
    QDomDocument doc;
    if (!doc.setContent(&f))
        return items;
    // <root><Content><Item>…</Item></Content>…
    QDomElement content = doc.documentElement().firstChildElement("Content");
    for (QDomElement e = content.firstChildElement("Item"); !e.isNull();
         e = e.nextSiblingElement("Item"))
        items.append(parseItem(e));
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
