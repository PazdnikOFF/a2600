#include "dicom/KDicomDatasetFormat.h"

#include <QFile>
#include <QXmlStreamReader>

namespace {
// Рекурсивный разбор содержимого текущего элемента до его закрытия.
void parseChildren(QXmlStreamReader &xml, QVector<KDicomDatasetFormat::Item> &out)
{
    while (!xml.atEnd()) {
        const auto tok = xml.readNext();
        if (tok == QXmlStreamReader::EndElement)
            return;                       // конец текущего контейнера
        if (tok != QXmlStreamReader::StartElement)
            continue;
        const QString tag = xml.name().toString();
        if (tag == "DcmItem") {
            KDicomDatasetFormat::Item it;
            it.name = xml.attributes().value("name").toString();
            it.isSequence = false;
            out.append(it);
            // DcmItem — лист; пропустить до его EndElement.
            xml.skipCurrentElement();
        } else if (tag == "SequenceItem") {
            KDicomDatasetFormat::Item it;
            it.name = xml.attributes().value("name").toString();
            it.isSequence = true;
            parseChildren(xml, it.children);   // вложенные узлы до </SequenceItem>
            out.append(it);
        }
    }
}
} // namespace

bool KDicomDatasetFormat::Load(const QString &xmlPath)
{
    items_.clear();
    QFile f(xmlPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QXmlStreamReader xml(&f);
    // Дойти до корня <root> и разобрать его содержимое.
    while (!xml.atEnd()) {
        if (xml.readNext() == QXmlStreamReader::StartElement &&
            xml.name() == QLatin1String("root")) {
            parseChildren(xml, items_);
            break;
        }
    }
    return !xml.hasError() && !items_.isEmpty();
}

int KDicomDatasetFormat::countRec(const QVector<Item> &v)
{
    int n = 0;
    for (const Item &i : v) {
        ++n;
        n += countRec(i.children);
    }
    return n;
}

int KDicomDatasetFormat::TotalCount() const
{
    return countRec(items_);
}

const KDicomDatasetFormat::Item *KDicomDatasetFormat::findRec(const QVector<Item> &v,
                                                              const QString &name)
{
    for (const Item &i : v) {
        if (i.name == name)
            return &i;
        if (const Item *r = findRec(i.children, name))
            return r;
    }
    return nullptr;
}

KDicomDatasetFormat::Item KDicomDatasetFormat::FindItem(const QString &name, bool *found) const
{
    if (const Item *r = findRec(items_, name)) {
        if (found) *found = true;
        return *r;
    }
    if (found) *found = false;
    return Item();
}

bool KDicomDatasetFormat::Contains(const QString &name) const
{
    return findRec(items_, name) != nullptr;
}
