#pragma once

#include <QDomDocument>
#include <QDomElement>

#include <string>

// База XML-подсистемы (реф. dialog/patient/reporttemplate/pugi/KMeaXMLBase.cpp, X-2600).
// Наследники (исчерпывающе, по xref на typeinfo): KTemplateCfg, KTemplateLibCfg,
// KTemplateParamCfg. KSysReportTempletCfg/KReportTemplateManager — не наследники,
// пользуются статикой (IsFileExist/ParseXML/ReplaceUserByLib).
//
// ОТЛИЧИЕ ОТ ОРИГИНАЛА: реф. использует вендоренный pugixml (measure::pugi), у нас
// его нет — API повторён на QDomDocument (Qt5::Xml). Семантика выровнена вручную:
//   * реф. грузит с parse_default|parse_comments|parse_declaration, БЕЗ parse_ws_pcdata
//     → whitespace-only текстовые узлы отбрасываются (QDom их хранит → фильтруем сами);
//   * FindDataNode берёт строго node_pcdata: CDATA НЕ считается;
//   * PI и DOCTYPE реф. отбрасывает (parse_pi/parse_doctype не выставлены);
//   * позиция/описание ошибки парсинга не логируются — только факт.
//
// Коды возврата (реф.): 1 — успех; -2 — исключение; -23 — copy failed;
// -40 — парсинг/файл не найден; -46 — несовпадение версии конфига.
class KMeaXMLBase
{
public:
    KMeaXMLBase() = default;
    virtual ~KMeaXMLBase() = default;

    // Контракт наследника — обе чисто виртуальные (реф. __cxa_pure_virtual).
    // База их НЕ диспетчирует: зовёт владелец (реф. KReportTemplateManager::InitModule).
    virtual int Check(const std::string &strLibFile, const std::string &strUsrFile) = 0;
    virtual int LoadCache() = 0;

    // Реф. возвращает -1 = «версия не объявлена»; ни один наследник не переопределяет.
    virtual int GetModuleVersion() const { return -1; }

    bool IsValid() const { return m_bValid; }   // реф.: всегда true (сброса в бинарнике нет)

    // Загрузка документа. Документ НЕ хранится в полях — отдаётся по ссылке.
    static int LoadXMLFile(const std::string &strFilePath, QDomDocument &doc);
    // Проверка «валиден ли файл» — документ создаётся локально и сразу уничтожается.
    static int ParseXML(const std::string &strFilePath);

    static bool IsFileExist(const std::string &strFilePath);
    // Восстановление пользовательского файла из библиотечного: user → user+".bak", затем copy.
    static int ReplaceUserByLib(const std::string &strLibFile, const std::string &strUserFile);

    int CheckVersion(const std::string &strFilePath) const;

    // Хелперы обхода (в реф. — члены, но this не используют).
    bool FindByName(const QDomElement &parent, const std::string &name, QDomElement &out) const;
    bool FindByValue(const QDomElement &parent, const std::string &name,
                     const std::string &value, QDomElement &out) const;
    bool FindByProperty(const QDomElement &parent, const std::string &name,
                        const std::string &attr, const std::string &value,
                        QDomElement &out) const;
    // Первый ребёнок-текст (не CDATA, не whitespace-only). Пустой узел, если нет.
    QDomNode FindDataNode(const QDomElement &parent) const;
    bool SetNodeValue(QDomElement node, const std::string &value) const;
    bool SetNodeProperty(QDomElement node, const std::string &attr, const std::string &value) const;

protected:
    bool m_bValid = true;   // +0x08
    // Реф. держит здесь ещё std::string (+0x10), к которой НИ ОДНА функция бинарника
    // не обращается — назначение невосстановимо, поэтому не воспроизводим.
};
