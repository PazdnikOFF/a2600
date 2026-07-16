#include "report/XmlParser.h"

#include <QDomProcessingInstruction>
#include <QFile>
#include <QTextStream>

bool XmlParser::LoadFromFile(const std::string &path)
{
    QFile f(QString::fromStdString(path));
    if (!f.open(QIODevice::ReadOnly)) {
        // реф. — load_file вернул бы false; description() ≈ ошибка чтения файла.
        m_strResult = "load file failed";
        return false;
    }
    QString err;
    const bool ok = m_doc.setContent(&f, &err);   // parse_default-эквивалент (как KMeaXMLBase)
    f.close();
    m_strResult = err.toStdString();   // реф. хранит description() парс-результата (пусто при успехе)
    return ok;
}

bool XmlParser::SaveToFile(const std::string &path) const
{
    QFile f(QString::fromStdString(path));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    QTextStream ts(&f);
    m_doc.save(ts, 1);   // реф. indent = "\t" + format_indent; Qt int-indent — ближайшее
    f.close();
    return true;
}

QDomElement XmlParser::GetRoot() const
{
    return m_doc.documentElement();
}

void XmlParser::SetDeclaration()
{
    // реф. append_child(node_declaration) с version="1.0", encoding="utf-8" (LOWERCASE).
    // QDom: PI с target "xml" → QDomDocument::save выводит её как XML-декларацию.
    QDomProcessingInstruction pi = m_doc.createProcessingInstruction(
        "xml", QStringLiteral("version=\"1.0\" encoding=\"utf-8\""));
    m_doc.appendChild(pi);
}
