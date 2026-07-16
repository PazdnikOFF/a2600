#pragma once

#include "report/KMeaXMLBase.h"
#include "report/KReportTemplateData.h"

#include <map>
#include <string>

// Библиотека шаблонов отчёта (реф. KTemplateLibCfg, X-2600). Наследник KMeaXMLBase,
// НЕ синглтон и НЕ UI (typeinfo: единственная база — KMeaXMLBase). Владелец —
// KReportTemplateManager::m_pTemplateLibCfg.
//
// ВЕТКА, ОТЛИЧНАЯ ОТ KTemplateCfg: работает с заводскими «кирпичами»
//   SubContentList.xml  → плоский пул ВСЕХ SubContent-блоков (LoadCache → m_data);
//   TemplateTypes.xml   → 5 готовых компоновок-«групп» (LoadCacheGroup → m_mapTemplateLibs).
// KTemplateCfg же читает склеенный FullTemplate/Template(NP-*).xml. Пересечение —
// только через static KTemplateCfg::ParseTemplateFile.
//
// «Группа» = именованный тип полного отчёта (ReportTemplateNP-1x4/2x2/2x3/4x1/nx3):
// имя = basename Ref до первой точки; каждая группа — свой список-файл, ссылающийся
// на подмножество SubContent/*.xml; все они мержатся в один KReportTemplateDataNew.
//
// Схема список-файлов СВОЯ (не root/TemplateConfig/Content/ItemConfig):
//   <root><Item Format="TemplateContent" Ref="HospitalTop.xml" /></root>
// а целевые файлы по Ref — уже обычная схема KTemplateCfg.
class KTemplateLibCfg : public KMeaXMLBase
{
public:
    KTemplateLibCfg() = default;

    // Реф. игнорирует ОБА аргумента (как и KTemplateCfg::Check) — строит 5 путей.
    int Check(const std::string &strLibFile, const std::string &strUsrFile) override;
    int LoadCache() override;      // SubContentList.xml → m_data (плоский пул блоков)
    virtual int LoadCacheGroup();  // доп. 6-й слот vtable: TemplateTypes.xml → группы

    // Коды реф.: -40 — не загрузился XML; 0 — нет корня "root"; 1 — успех.
    // 3-й аргумент в реф. передаётся по значению и НЕ используется — сохраняем 1:1.
    int LoadTemplateLib(const std::string &strFile, KReportTemplateDataNew &data,
                        std::string strUnused);
    int LoadTemplateLibs(const std::string &strFile);

    // ВНИМАНИЕ (реф.): при промахе возвращает НЕ nullptr, а ссылку на m_data.
    KReportTemplateDataNew *GetTemplateLib(const std::string &strName);

    // Обе перегрузки НИКУДА НЕ ПИШУТ (сверено дизасмом): делают копию, прогоняют
    // RemoveNotUserItem, копию выбрасывают и перечитывают с диска. Сохранения нет.
    int UpdateTemplateLib(const std::string &strName, const KReportTemplateDataNew &data);
    int UpdateTemplateLib(const KReportTemplateDataNew &data);

    // Безусловно чистит m_mapConfigs; из m_mapItemConfigs оставляет только
    // m_bUserDefine; из дерева — только элементы, чей m_strID попал в множество
    // префиксов длиной >= 2 (реф. — this не использует).
    bool RemoveNotUserItem(KReportTemplateDataNew &data);

    const KReportTemplateDataNew &Data() const { return m_data; }
    const std::map<std::string, KReportTemplateDataNew> &TemplateLibs() const
    {
        return m_mapTemplateLibs;
    }

private:
    std::string m_strTemplateLibFile;    // +0x30  RO + …/template/SubContentList.xml
    std::string m_strRoSubContentDir;    // +0x50  RO + …/template/SubContent
    std::string m_strUserSubContentDir;  // +0x70  Usr + …/template/SubContent
    std::string m_strRoTemplateDir;      // +0x90  RO + …/template/
    std::string m_strTemplateTypesFile;  // +0xb0  RO + …/template/TemplateTypes.xml
    KReportTemplateDataNew m_data;                             // +0xd0
    std::map<std::string, KReportTemplateDataNew> m_mapTemplateLibs;   // +0x148
};
