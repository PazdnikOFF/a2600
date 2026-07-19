#include "report/KSysReportTempletControl.h"
#include "report/KSysReportTempletModel.h"

#include <QDate>
#include <QDebug>

#include <cassert>

// Синглтон (реф. GetInstance = call_once + heap shared_ptr; здесь Meyers-static —
// эквивалентно по наблюдаемому поведению).
KSysReportTempletControl *KSysReportTempletControl::GetInstance()
{
    static KSysReportTempletControl inst;
    return &inst;
}

KSysReportTempletControl::KSysReportTempletControl()
    : m_model(new KSysReportTempletModel())   // реф. ctor: new KSysReportTempletModel
{
}

KSysReportTempletControl::~KSysReportTempletControl()
{
    delete m_model;
}

void KSysReportTempletControl::Init()
{
    assert(m_model != nullptr);
    m_model->Init();
    // выбранный шаблон := дефолт текущего департамента (на первом вызове dept пуст).
    m_selectedTemplet = m_model->GetDefalutTempletNameByDept(m_selectedDept);
}

void KSysReportTempletControl::Save()
{
    assert(m_model != nullptr);
    m_model->Save();
}

void KSysReportTempletControl::Reload()
{
    assert(m_model != nullptr);
    m_model->Reload();
}

void KSysReportTempletControl::LoadDefault()
{
    assert(m_model != nullptr);
    m_model->LoadDefault();
    m_selectedTemplet = m_model->GetDefalutTempletNameByDept(m_selectedDept);
}

void KSysReportTempletControl::DiscardChange()
{
    assert(m_model != nullptr);
    m_model->DiscardChange();
}

KTempletBaseInfo KSysReportTempletControl::GetSelectedTempletInfo() const
{
    assert(m_model != nullptr);
    KTempletBaseInfo info;
    m_model->GetTempletInfoByName(m_selectedTemplet, info);   // bool-результат отбрасывается
    return info;
}

bool KSysReportTempletControl::IsExist(const std::string &name) const
{
    assert(m_model != nullptr);
    return m_model->IsExist(name);
}

bool KSysReportTempletControl::GetTempletInfoByName(const std::string &name,
                                                    KTempletBaseInfo &out) const
{
    assert(m_model != nullptr);
    return m_model->GetTempletInfoByName(name, out);
}

void KSysReportTempletControl::GetTempletInfosByDept(const std::string &dept,
                                                     QVector<KTempletBaseInfo> &out) const
{
    assert(m_model != nullptr);
    m_model->GetTempletInfosByDept(dept, out);
}

void KSysReportTempletControl::OnDeptChanged(const std::string &dept)
{
    assert(m_model != nullptr);
    m_selectedDept = dept;
    if (m_selectedDept.compare("KW_ALL") == 0)   // сентинел «все департаменты» — выбор шаблона не трогаем
        return;
    m_selectedTemplet = m_model->GetDefalutTempletNameByDept(m_selectedDept);
}

void KSysReportTempletControl::OnSelectedTempletChanged(const std::string &t)
{
    // реф.: без ассерта и без модели — просто присвоение.
    m_selectedTemplet = t;
}

void KSysReportTempletControl::SetSelectedTempletDefault()
{
    assert(m_model != nullptr);
    m_model->SetDefault(m_selectedTemplet, m_selectedDept, true);   // (имя, департамент, флаг)
}

void KSysReportTempletControl::RenameSelectedTemplet(const std::string &newName)
{
    assert(m_model != nullptr);
    m_model->RenameTemplate(m_selectedTemplet, newName);
    m_selectedTemplet = newName;   // выбор следует за переименованием
}

void KSysReportTempletControl::DeleteSelectedTemplet()
{
    // QUIRK (реф.): ПУСТОЕ тело — ни ассерта, ни вызова модели. НЕ «чинить».
}

void KSysReportTempletControl::GetTemplateCfgByName(const std::string &name,
                                                    KReportTemplateDataNew &out)
{
    assert(m_model != nullptr);
    m_model->GetTemplateCfgByName(name, out);
}

void KSysReportTempletControl::GetSelectedTemplateCfg(KReportTemplateDataNew &out)
{
    assert(m_model != nullptr);
    GetTemplateCfgByName(m_selectedTemplet, out);
}

void KSysReportTempletControl::SaveSingleTemplateCfg(const KTempletBaseInfo &info,
                                                     const KReportTemplateDataNew &cfg)
{
    assert(m_model != nullptr);
    qInfo("now save templet(%s) cfg", info.TempletName().toUtf8().constData());
    m_model->SaveSingleTemplatetCfg(info, cfg);
}

bool KSysReportTempletControl::AddUserDefineDept(const std::string &dept)
{
    assert(m_model != nullptr);
    qInfo("now AddUserDefineDept(%s) cfg", dept.c_str());
    return m_model->AddUserDefineDept(dept);
}

bool KSysReportTempletControl::DeleteUserDefineDept(const std::string &dept)
{
    assert(m_model != nullptr);
    qInfo("now DeleteUserDefineDept(%s) cfg", dept.c_str());
    return m_model->DeleteUserDefineDept(dept);
}

bool KSysReportTempletControl::UpdateUserDefineItem(const KReportTemplateDataNew &data)
{
    assert(m_model != nullptr);
    qInfo("now UpdateUserDefineItem cfg");
    m_model->UpdateUserDefineItem(data);
    m_model->Save();
    return true;
}

std::string KSysReportTempletControl::GetSelectedDept() const
{
    // QUIRK (реф.): без ассерта/модели/трансляции — сырой m_selectedDept (в т.ч. "KW_ALL").
    return m_selectedDept;
}

KTempletBaseInfo KSysReportTempletControl::GetCopyTempletInfo(const KTempletBaseInfo &src,
                                                              const std::string &newName) const
{
    KTempletBaseInfo info;
    info.SetTempletName(QString::fromStdString(newName));   // QUIRK: без уникализации
    const QString date = QDate::currentDate().toString("yyyy-MM-dd");
    info.SetModifyDate(date);
    info.SetFactory(false);
    for (const QString &dept : src.GetDepts())              // QUIRK: default-флаг всегда false
        info.AddDept(dept, false);
    qInfo("copy info: name is %s, date is %s, factory is %d",
          info.TempletName().toUtf8().constData(), date.toUtf8().constData(),
          info.IsFactory() ? 1 : 0);
    return info;
}

void KSysReportTempletControl::CopySelectedTemplate(const std::string &newName,
                                                    KTempletBaseInfo &outInfo,
                                                    KReportTemplateDataNew &outCfg)
{
    assert(m_model != nullptr);
    KTempletBaseInfo tmp;
    m_model->GetTempletInfoByName(m_selectedTemplet, tmp);
    outInfo = GetCopyTempletInfo(tmp, newName);
    // cfg берётся у ИСХОДНОГО выбранного шаблона (реф. — m_selectedTemplet, не newName).
    m_model->GetTemplateCfgByName(m_selectedTemplet, outCfg);
}
