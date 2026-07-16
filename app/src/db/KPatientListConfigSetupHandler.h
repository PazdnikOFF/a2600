#pragma once

#include <map>
#include <memory>
#include <string>

class KConfig;

// Видимость колонок списка пациентов (реф. dialog/patient/patientlist/
// KPatientListConfigSetupHandler.cpp, X-2600).
//
// Файл: <data>/protected/patientsetup.ini, единственная секция [ShowOnMainUi].
// В отличие от KExamListConfigHandler (QSettings) — фасад над KConfig без своего
// состояния: чтение ленивое при каждом вызове, запись только в память, на диск —
// SaveConfig(). Дефолт ВСЕХ bool-колонок — true.
//
// Опечатки оригинала сохранены намеренно: IsShowPatietID, IsShowRegisterNumer.
class KPatientListConfigSetupHandler
{
public:
    static KPatientListConfigSetupHandler *GetInstance();   // реф. std::call_once

    // --- Видимость колонок ([ShowOnMainUi], дефолт true) ---
    bool IsShowPatietID();                  // patientid       (sic — опечатка реф.)
    void SetIsShowPatietID(bool v);
    bool IsShowApplicant();                 // applicant
    void SetIsShowApplicant(bool v);
    bool IsShowApplicantDate();             // applicantdate
    void SetIsShowApplicantDate(bool v);
    bool IsShowBirthday();                  // birthday
    void SetIsShowBirthday(bool v);
    bool IsShowTelephone();                 // telephone
    void SetIsShowTelephone(bool v);
    bool IsShowSickbedNum();                // bedno
    void SetIsShowSickbedNum(bool v);
    bool IsShowRegisterNumer();             // registernumber  (sic — опечатка реф.)
    void SetIsShowRegisterNumer(bool v);
    bool IsShowSelfDefineField1();          // userdefined1
    void SetIsShowSelfDefineField1(bool v);
    bool IsShowSelfDefineField2();          // userdefined2
    void SetIsShowSelfDefineField2(bool v);

    // --- Заголовки пользовательских полей (дефолт "") ---
    std::string GetSelfDefineField1Title(); // userdefinedtitle1
    void SetSelfDefineField1Title(const std::string &v);
    std::string GetSelfDefineField2Title(); // userdefinedtitle2
    void SetSelfDefineField2Title(const std::string &v);

    // Маппинг «имя колонки → 0/1». Ровно 7 записей (self-define полей тут НЕТ);
    // ключи map намеренно НЕ совпадают с ключами .ini (реф.).
    void GetColumnIsShow(std::map<std::string, int> &out);

    void SaveConfig();   // реф. → KConfig::Save()

private:
    KPatientListConfigSetupHandler();
    std::shared_ptr<KConfig> m_config;   // +0x00 (единственное поле)
};
