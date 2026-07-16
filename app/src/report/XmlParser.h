#pragma once

#include <QDomDocument>
#include <QDomElement>

#include <string>

// Тонкая обёртка XML-документа (реф. XmlParser, X-2600, sizeof 0x28). Реф. на вендоренном
// pugixml (pugi::xml_document* + std::string описания парсинга); off-device — на QDomDocument
// (Qt5::Xml), как это уже сделано для KMeaXMLBase. GetRoot отдаёт QDomElement (реф. —
// pugi::xml_node от document_element()).
//
// Отличия off-device: документ — value-член (реф. heap-указатель); SetDeclaration невиртуально
// не-const (реф. const через указатель); indent при сохранении — Qt int (реф. литерал таб "\t").
class XmlParser
{
public:
    XmlParser() = default;

    // load_file(path, parse_default, encoding_auto); m_strResult = описание; возврат успех.
    bool LoadFromFile(const std::string &path);
    // save_file(path, "\t", format_indent, encoding_auto). Возврат успех.
    bool SaveToFile(const std::string &path) const;

    QDomElement GetRoot() const;                             // document_element()
    const std::string &GetParseResult() const { return m_strResult; }
    void SetDeclaration();   // append <?xml version="1.0" encoding="utf-8"?> (реф. append_child)

private:
    QDomDocument m_doc;         // реф. pugi::xml_document* @+0x20 — off-device value
    std::string  m_strResult;   // реф. @+0x00 — описание последнего парсинга
};
