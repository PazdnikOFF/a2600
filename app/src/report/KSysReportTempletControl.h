#pragma once

#include "report/KReportTemplateData.h"       // KReportTemplateDataNew
#include "report/KSysReportTempletCfg.h"       // KTempletBaseInfo

#include <QVector>

#include <string>

class KSysReportTempletModel;

// Контроллер каталога шаблонов отчёта (реф. KSysReportTempletControl,
// dialog/patient/reporttemplate/template/, X-2600). Синглтон-обёртка над
// KSysReportTempletModel, добавляющая состояние UI-выбора: выбранный департамент и
// выбранный шаблон. Почти все методы делегируют модели; часть попутно обновляет выбор.
// Off-device (чистая логика + QDate).
//
// Реф.-квирки, СОХРАНЁННЫЕ намеренно (НЕ «чинить»):
//   * DeleteSelectedTemplet — ПУСТОЕ тело (ret; ни модели, ни ассерта).
//   * GetCopyTempletInfo — НЕ уникализирует имя (берёт newName как есть) и сбрасывает
//     per-dept флаг default в false.
//   * GetSelectedDept — возвращает m_selectedDept ДОСЛОВНО (сентинел "KW_ALL" не транслируется).
class KSysReportTempletControl
{
public:
    static KSysReportTempletControl *GetInstance();

    void Init();                 // model->Init(); выбранный шаблон := дефолт текущего департамента
    void Save();                 // model->Save()
    void Reload();               // model->Reload()
    void LoadDefault();          // model->LoadDefault(); выбранный шаблон := дефолт департамента
    void DiscardChange();        // model->DiscardChange()

    KTempletBaseInfo GetSelectedTempletInfo() const;
    bool IsExist(const std::string &name) const;
    bool GetTempletInfoByName(const std::string &name, KTempletBaseInfo &out) const;
    void GetTempletInfosByDept(const std::string &dept, QVector<KTempletBaseInfo> &out) const;

    void OnDeptChanged(const std::string &dept);            // выбор департамента (+дефолт шаблона)
    void OnSelectedTempletChanged(const std::string &t);    // просто m_selectedTemplet = t
    void SetSelectedTempletDefault();                       // model->SetDefault(templet, dept, true)
    void RenameSelectedTemplet(const std::string &newName); // rename + выбор следует за именем
    void DeleteSelectedTemplet();                           // QUIRK: пустое тело

    void GetTemplateCfgByName(const std::string &name, KReportTemplateDataNew &out);
    void GetSelectedTemplateCfg(KReportTemplateDataNew &out);
    void SaveSingleTemplateCfg(const KTempletBaseInfo &info, const KReportTemplateDataNew &cfg);

    bool AddUserDefineDept(const std::string &dept);
    bool DeleteUserDefineDept(const std::string &dept);
    bool UpdateUserDefineItem(const KReportTemplateDataNew &data);

    std::string GetSelectedDept() const;                   // QUIRK: сырой m_selectedDept
    // QUIRK: имя не уникализируется, default-флаг департаментов сбрасывается в false.
    KTempletBaseInfo GetCopyTempletInfo(const KTempletBaseInfo &src, const std::string &newName) const;
    // Заполняет outInfo (копия выбранного под newName) + outCfg (cfg ИСХОДНОГО выбранного).
    void CopySelectedTemplate(const std::string &newName, KTempletBaseInfo &outInfo,
                              KReportTemplateDataNew &outCfg);

private:
    KSysReportTempletControl();
    ~KSysReportTempletControl();
    KSysReportTempletControl(const KSysReportTempletControl &) = delete;
    KSysReportTempletControl &operator=(const KSysReportTempletControl &) = delete;

    KSysReportTempletModel *m_model = nullptr;   // +0x00 — heap
    std::string m_selectedDept;                  // +0x08
    std::string m_selectedTemplet;               // +0x28
};
