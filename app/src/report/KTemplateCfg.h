#pragma once

#include "report/KMeaXMLBase.h"
#include "report/KReportTemplateData.h"

#include <map>
#include <string>
#include <vector>

// Загрузчик/кэш шаблонов отчёта (реф. dialog/patient/reporttemplate/template/
// TemplateCfg/KTemplateCfg.cpp, X-2600). Наследник KMeaXMLBase, НЕ синглтон —
// владельцы держат его полем (реф. KSysReportTempletCfg, KReportTemplateManager::
// InitModule, KTemplateLibCfg).
//
// Работает с ВЕТКОЙ FullTemplate: <dir>/Template(<name>).xml, где <name> —
// NP-1x4|NP-2x2|NP-2x3|NP-4x1|NP-nx3 (5 реальных файлов в syspreset).
// Это НЕ те же файлы, что читает наш KReportTemplate (ReportTemplateNP-*.xml +
// SubContent/ — заводские «кирпичи»); схема одна, ветки разные.
//
// UI не тянет вовсе (несмотря на путь dialog/…) — весь класс off-device.
class KTemplateCfg : public KMeaXMLBase
{
public:
    KTemplateCfg();

    // Реф. переопределяет обе чисто виртуальные базы.
    int Check(const std::string &strLibFile, const std::string &strUsrFile) override;
    int LoadCache() override;   // реф. — заглушка `return 1` (кэш ленивый)

    void GetUserConfigPath(std::string &outDir, std::string &outName) const;

    // "Template(" + name + ").xml"
    static std::string GetTemplateFileName(const std::string &strName);
    // QDir::entryList + regexp "Template\((.+)\).xml" → captured(1)
    static std::vector<std::string> GetTemplateFiles(const std::string &strDir);
    std::vector<std::string> GetLibTemplateFiles() const;
    std::vector<std::string> GetUserTemplateFiles() const;

    // Кэш → при промахе разбор файла: сначала user-ветка, затем фолбэк на RO.
    int GetTemplateCfg(const std::string &strName, KReportTemplateDataNew &out);
    int GetSubTemplateData(const std::string &strName, KReportTemplateDataNew &out);
    int UpdateTemplateCfg(const std::string &strName, const KReportTemplateDataNew &data);
    int DeleteTemplateCfg(const std::string &strName);

    // --- Разбор/запись (в реф. static: this не используется) ---
    static int ParseTemplateFile(const std::string &strFile, KReportTemplateDataNew &out);
    static int SaveTemplateFile(const std::string &strFile, const KReportTemplateDataNew &data);

    static void ParseTemplateConfig(const QDomElement &node,
                                    std::map<std::string, std::string> &out);
    static void SaveTemplateConfig(QDomElement &node,
                                   const std::map<std::string, std::string> &in);
    static void ParseTemplateContent(const QDomElement &node, const std::string &strParentPath,
                                     std::list<KReportTemplateItem> &out);
    static void SaveTemplateContent(QDomElement &node,
                                    const std::list<KReportTemplateItem> &in);
    static void ParseTemplateItemConfig(const QDomElement &node,
                                        std::map<std::string, KReportTemplateItemConfig> &out);
    static void SaveTemplateItemConfig(QDomElement &node,
                                       const std::map<std::string, KReportTemplateItemConfig> &in);

private:
    std::string m_strRODir;     // +0x30 — каталог FullTemplate в RO-ветке (syspreset)
    std::string m_strUserDir;   // +0x50 — каталог FullTemplate в user-ветке (userpreset)
    // Реф. держит ТРИ map<string, KReportTemplateDataNew> (+0x70/+0xa0/+0xd0), но
    // доказан только @0xa0 (его читает GetTemplateCfg). Роли двух других из дизасма
    // не восстановлены — не воспроизводим, чтобы не фантазировать.
    std::map<std::string, KReportTemplateDataNew> m_mapCache;   // +0xa0
};
