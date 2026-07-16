#pragma once

#include <memory>
#include <string>

class KConfig;
class QDate;

// Настройки запроса/показа списка Worklist (реф. dialog/patient/patientlist/
// KWorklistConfigSetupHandler.cpp, X-2600 — файл Worklist, класс WorkList).
//
// Файл: <data>/protected/worklistsetup.ini, секция [ShowOnMainUi].
// Семантика ИНАЯ, чем у KPatientListConfigSetupHandler: Is*On-ключи — видимость
// поля фильтра, а «голые» ключи (patientid/patientname/…) — сами ЗНАЧЕНИЯ фильтра
// запроса, а не колонки. Дефолты: bool → true, строки → "", Equipment → 0.
class KWorkListConfigSetupHandler
{
public:
    static KWorkListConfigSetupHandler *GetInstance();   // реф. std::call_once

    bool IsShowPatientID();                       // IsPatientidOn      (дефолт true)
    void SetIsShowPatientID(bool v);
    std::string GetShowPatientID();               // patientid          (дефолт "")
    void SetShowPatientID(const std::string &v);

    bool IsShowPatientName();                     // IsPatientnameOn
    void SetIsShowPatientName(bool v);
    std::string GetShowPatientName();             // patientname
    void SetShowPatientName(const std::string &v);

    bool IsShowRegisterNumber();                  // IsRegisterNumberOn
    void SetIsShowRegisterNumber(bool v);
    std::string GetShowRegisterNumber();          // RegisterNumber (sic — с заглавной)
    void SetShowRegisterNumber(const std::string &v);

    bool IsShowPlantime();                        // IsPlantimeOn
    void SetIsShowPlantime(bool v);
    // plantimestart/plantimeend, формат "yyyy-MM-dd" (литерал реф.)
    void GetShowPlantime(QDate &start, QDate &end);
    void SetShowPlantime(const QDate &start, const QDate &end);

    bool IsShowInspectEquipment();                // IsEquipmentOn
    void SetIsShowInspectEquipment(bool v);
    int  GetShowInspectEquipment();               // Equipment          (дефолт 0)
    void SetShowInspectEquipment(const int &v);

    void SaveConfig();   // реф. → KConfig::Save()

private:
    KWorkListConfigSetupHandler();
    std::shared_ptr<KConfig> m_config;   // +0x00 (единственное поле)
};
