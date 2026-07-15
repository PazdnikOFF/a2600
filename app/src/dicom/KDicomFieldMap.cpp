#include "dicom/KDicomFieldMap.h"

#include <QFile>
#include <QXmlStreamReader>
#include <QSet>

bool KDicomFieldMap::Load(const QString &xmlPath)
{
    fields_.clear();
    records_.clear();
    QFile f(xmlPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QXmlStreamReader xml(&f);
    while (!xml.atEnd()) {
        const auto tok = xml.readNext();
        if (tok != QXmlStreamReader::StartElement)
            continue;
        const QStringRef name = xml.name();
        if (name == QLatin1String("Record")) {
            const auto a = xml.attributes();
            Record rec;
            rec.type        = a.value("type").toString();
            rec.subGroup    = a.value("SubGroup").toString();
            rec.datasetPath = a.value("DatasetPath").toString();
            records_.append(rec);
        } else if (name == QLatin1String("Field")) {
            const auto a = xml.attributes();
            Field fld;
            fld.dbname      = a.value("dbname").toString();
            fld.datasetPath = a.value("DatasetPath").toString();
            fld.format      = a.value("format").toString();
            if (!fld.dbname.isEmpty()) {
                fields_.append(fld);
                if (!records_.isEmpty())     // поле принадлежит текущей записи
                    records_.last().fields.append(fld);
            }
        }
    }
    return !xml.hasError() && !fields_.isEmpty();
}

KDicomFieldMap::Record KDicomFieldMap::RecordByType(const QString &type, bool *found) const
{
    for (const Record &r : records_)
        if (r.type == type) {
            if (found) *found = true;
            return r;
        }
    if (found) *found = false;
    return Record();
}

QVector<QString> KDicomFieldMap::ColumnNames() const
{
    QVector<QString> cols;
    QSet<QString> seen;
    for (const Field &f : fields_) {
        if (seen.contains(f.dbname))
            continue;             // в XML встречаются повторы (CodeValue и т.п.)
        seen.insert(f.dbname);
        cols.append(f.dbname);
    }
    return cols;
}
