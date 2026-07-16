#pragma once

#include "report/KMeaXMLBase.h"

#include <map>
#include <string>

// Параметры шаблона отчёта (реф. KTemplateParamCfg, X-2600) — ТРЕТИЙ и последний
// наследник KMeaXMLBase. НЕ синглтон, НЕ UI.
//
// ⚠️ В ЭТОЙ ПРОШИВКЕ КЛАСС МЁРТВЫЙ (вестигиальный): ни одного xref (ctor/Check/
// LoadCache/GetTemplateParam не вызываются нигде), KReportTemplateManager::InitModule
// создаёт только KTemplateCfg и KTemplateLibCfg, а файла ReportTemplateParam.xml
// (как и каталога report/ReportTemplate/) в прошивке НЕТ. Реализован для полноты
// покрытия; на рантайм не влияет.
//
// Хранит плоский двухуровневый key-value: группа → (Name → Value).
// Схема СВОЯ (не root/TemplateConfig/Content/ItemConfig):
//   <root>
//     <ЛюбоеИмяГруппы>            <!-- имя элемента = ключ группы, фильтра нет -->
//       <Item Name="ключ" Value="значение"/>
//     </ЛюбоеИмяГруппы>
//   </root>
class KTemplateParamCfg : public KMeaXMLBase
{
public:
    KTemplateParamCfg() = default;

    // ОТЛИЧИЕ ОТ СИБЛИНГОВ: здесь arg1 ИСПОЛЬЗУЕТСЯ как базовый каталог
    // (KTemplateCfg/KTemplateLibCfg игнорируют оба и берут пути из KEnvConfig;
    // KEnvConfig в этом TU не вызывается вовсе). arg2 игнорируется. Всегда 1.
    int Check(const std::string &strBaseDir, const std::string &strUnused) override;
    int LoadCache() override;   // clear() кэша + ParseParamFile (код проброшен)

    // 1 — найдено; 0 — промах (strValue НЕ трогается, дефолт не вставляется).
    // В отличие от KTemplateLibCfg::GetTemplateLib здесь честный промах.
    int GetTemplateParam(const std::string &strGroup, const std::string &strKey,
                         std::string &strValue);

    // Коды: -40 — XML не загрузился; 0 — нет корня "root"; 1 — успех.
    // mapOut внутри НЕ очищается (чистит вызывающий); при ошибке остаётся как был.
    // Пустые группы (без валидных Item) отбрасываются.
    static int ParseParamFile(const std::string &strFile,
                              std::map<std::string, std::map<std::string, std::string>> &mapOut);
    // Внутри группы — только элементы с именем строго "Item"; берутся атрибуты
    // Name/Value, и лишь если ОБА непустые. Всегда 1 (возврат реф. игнорирует).
    static int ParseParamGroup(const QDomElement &nodeGroup,
                               std::map<std::string, std::string> &mapOut);

    const std::map<std::string, std::map<std::string, std::string>> &Params() const
    {
        return m_mapParams;
    }
    const std::string &ParamFile() const { return m_strParamFile; }

private:
    std::string m_strParamFile;   // +0x30
    std::map<std::string, std::map<std::string, std::string>> m_mapParams;   // +0x50
};
