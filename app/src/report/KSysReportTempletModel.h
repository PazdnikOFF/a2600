#pragma once

#include "report/KReportTemplateData.h"     // KReportTemplateDataNew / KReportTemplateItem
#include "report/KSysReportTempletCfg.h"     // KTempletBaseInfo

#include <QVector>

#include <map>
#include <string>
#include <vector>

// Модель каталога шаблонов отчёта (реф. KSysReportTempletModel,
// dialog/patient/reporttemplate/template/, X-2600). Поверх файлового каталога
// KSysReportTempletCfg держит рабочую in-memory копию инфо-о-шаблонах, кэш cfg по
// имени, список удалённых имён и буфер новых либ-элементов; Save() сбрасывает всё
// в KSysReportTempletCfg + KTemplateLibCfg. Off-device (чистая логика).
//
// Реф.-квирки, СОХРАНЁННЫЕ намеренно (НЕ «чинить»):
//   * GetDefalutTempletNameByDept ВОЗВРАЩАЕТ АРГУМЕНТ dept, а не вычисленное имя
//     шаблона (sret инициализируется копией dept и не переписывается); плюс
//     побочный эффект — при отсутствии дефолта назначает report_preview::NP_nx3.
//   * SetDefault(templetName, dept, bool) — порядок аргументов (имя, департамент,
//     флаг); clear-цикл снимает default у ВСЕХ шаблонов департамента даже при bool==false.
//   * DeleteTemplate удаляет, только если имя есть И в infos, И в cfg-кэше; иначе
//     логирует «... but not in cfg lib» и ПРОДОЛЖАЕТ обход.
class KSysReportTempletModel
{
public:
    KSysReportTempletModel();
    ~KSysReportTempletModel();

    void Init();                 // ClearCfgCache + перечитать инфо из KSysReportTempletCfg
    void ClearCfgCache();        // очистить m_vecTempletInfos + m_mapCfgCache + m_delList
    void LoadDefault();          // cfg->LoadDefault() + Init()
    void DiscardChange();        // cfg->LoadUserXML() + Init()
    void Reload();               // cfg->Reload() + сверка новых имён с in-memory
    void Save();                 // SaveTempletInfos + SaveTemplateCfg + (либ-буфер)

    bool IsExist(const std::string &name) const;
    bool GetTempletInfoByName(const std::string &name, KTempletBaseInfo &out) const;
    void GetTempletInfosByDept(const std::string &dept, QVector<KTempletBaseInfo> &out) const;
    bool GetTemplateCfgByName(const std::string &name, KReportTemplateDataNew &out);

    // QUIRK: возвращает АРГУМЕНТ dept (см. заголовок класса), НЕ имя шаблона.
    std::string GetDefalutTempletNameByDept(const std::string &dept);
    // QUIRK: аргументы (templetName, dept, isDefault).
    void SetDefault(const std::string &templetName, const std::string &dept, bool isDefault);

    bool AddUserDefineDept(const std::string &dept);
    bool DeleteUserDefineDept(const std::string &dept);
    bool UpdateUserDefineItem(const KReportTemplateDataNew &data);

    void DeleteTemplate(const std::string &name);
    void RenameTemplate(const std::string &from, const std::string &to);
    void SaveSingleTemplatetCfg(const KTempletBaseInfo &info, const KReportTemplateDataNew &data);

    // Рекурсивный сбор кастомных под-элементов, удаляемых вместе с item (реф. возвращает
    // bool; out — вектор УКАЗАТЕЛЕЙ на элементы дерева `item`, валидны пока живо дерево).
    // Правило: item — кандидат на удаление, если он НЕ найден FindConstRefItem (=user-define),
    // ЛИБО у него есть дети и ВСЕ они попали на удаление.
    bool GetDelUserDefineItem(KReportTemplateItem *item,
                              std::vector<KReportTemplateItem *> &out,
                              const KReportTemplateDataNew &data);

private:
    QVector<KTempletBaseInfo> m_vecTempletInfos;                     // +0x00
    std::map<std::string, KReportTemplateDataNew> m_mapCfgCache;     // +0x18
    std::vector<std::string> m_delList;                             // +0x48
    KReportTemplateDataNew m_tmpData;                              // +0x60
};
