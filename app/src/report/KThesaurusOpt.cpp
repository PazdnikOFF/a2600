#include "report/KThesaurusOpt.h"
#include "sys/KSystem.h"

#include <QDir>
#include <QFile>
#include <QDomDocument>
#include <QTextStream>
#include <QSet>

QString KThesaurusOpt::GetFileNameByEndoscopeType(ScopeClass cls)
{
    // Реф. GetFileNMameByEndoscopeType (switch по KScopeClass → имя файла словаря).
    switch (cls) {
    case Gastroscopy:     return "Gastroscopy.xml";
    case Colonoscopy:     return "Colonoscopy.xml";
    case Duodenoscope:    return "Duodenoscope.xml";
    case Bronchoscope:    return "Bronchoscope.xml";
    case Choledochoscopy: return "Choledochoscopy.xml";
    case Noselarynxscope: return "Noselarynxscope.xml";
    }
    return "Gastroscopy.xml";
}

KThesaurusOpt::KThesaurusOpt(const QString &lang, const QString &thesaurusRoot)
    : lang_(lang), root_(thesaurusRoot)
{
    if (root_.isEmpty())
        root_ = QDir(KSystem::SystemPath())
            .absoluteFilePath("presetdata/syspreset/patient/thesaurus");
}

QString KThesaurusOpt::filePathFor(ScopeClass cls) const
{
    return QDir(root_).absoluteFilePath(lang_ + "/" + GetFileNameByEndoscopeType(cls));
}

bool KThesaurusOpt::Load(ScopeClass cls)
{
    return LoadFile(filePathFor(cls));
}

bool KThesaurusOpt::LoadFile(const QString &xmlPath)
{
    records_.clear();
    loadedFile_ = xmlPath;
    QFile f(xmlPath);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    QDomDocument doc;
    if (!doc.setContent(&f))
        return false;
    // <list><record>…</record></list>
    QDomElement root = doc.documentElement();
    for (QDomElement e = root.firstChildElement("record"); !e.isNull();
         e = e.nextSiblingElement("record")) {
        Record r;
        r.diseaGroup      = e.firstChildElement("diseagroup").text();
        r.checkedItemName = e.firstChildElement("checkedItemname").text();
        r.diseaName       = e.firstChildElement("diseaname").text();
        r.examFinding     = e.firstChildElement("examfinding").text();
        r.diagResult      = e.firstChildElement("diagresult").text();
        r.grid            = e.firstChildElement("grid").text();
        records_.append(r);
    }
    return true;
}

QStringList KThesaurusOpt::Groups() const
{
    QStringList groups;
    QSet<QString> seen;
    for (const Record &r : records_) {
        if (r.diseaGroup.isEmpty() || seen.contains(r.diseaGroup))
            continue;
        seen.insert(r.diseaGroup);
        groups << r.diseaGroup;
    }
    return groups;
}

QVector<KThesaurusOpt::Record> KThesaurusOpt::RecordsByGroup(const QString &group) const
{
    QVector<Record> out;
    for (const Record &r : records_)
        if (r.diseaGroup == group)
            out.append(r);
    return out;
}

bool KThesaurusOpt::RecordByGrid(const QString &grid, Record &out) const
{
    for (const Record &r : records_)
        if (r.grid == grid) { out = r; return true; }
    return false;
}

bool KThesaurusOpt::RecordByDisease(const QString &diseaName, Record &out) const
{
    for (const Record &r : records_)
        if (r.diseaName == diseaName) { out = r; return true; }
    return false;
}

QString KThesaurusOpt::AddDiseaseContent(const QString &group, const QString &diseaName,
                                         const QString &examFinding, const QString &diagResult)
{
    // Новый grid = max+1 (реф. уникальный ID записи).
    int maxGrid = 1000;
    for (const Record &r : records_) {
        bool ok = false; const int g = r.grid.toInt(&ok);
        if (ok && g > maxGrid) maxGrid = g;
    }
    Record r;
    r.diseaGroup = group;
    r.diseaName = diseaName;
    r.examFinding = examFinding;
    r.diagResult = diagResult;
    r.grid = QString::number(maxGrid + 1);
    if (!records_.isEmpty())
        r.checkedItemName = records_.first().checkedItemName;
    records_.append(r);
    return r.grid;
}

bool KThesaurusOpt::DelDiseaseContentByGrid(const QString &grid)
{
    for (int i = 0; i < records_.size(); ++i)
        if (records_[i].grid == grid) { records_.remove(i); return true; }
    return false;
}

bool KThesaurusOpt::Save() const
{
    if (loadedFile_.isEmpty())
        return false;
    QDomDocument doc;
    doc.appendChild(doc.createProcessingInstruction("xml", "version=\"1.0\""));
    QDomElement list = doc.createElement("list");
    doc.appendChild(list);
    auto addField = [&](QDomElement &rec, const QString &tag, const QString &val) {
        QDomElement e = doc.createElement(tag);
        e.appendChild(doc.createTextNode(val));
        rec.appendChild(e);
    };
    for (const Record &r : records_) {
        QDomElement rec = doc.createElement("record");
        addField(rec, "diseagroup", r.diseaGroup);
        addField(rec, "checkedItemname", r.checkedItemName);
        addField(rec, "diseaname", r.diseaName);
        addField(rec, "examfinding", r.examFinding);
        addField(rec, "diagresult", r.diagResult);
        addField(rec, "grid", r.grid);
        list.appendChild(rec);
    }
    QFile f(loadedFile_);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream ts(&f);
    ts << doc.toString(1);
    return true;
}
