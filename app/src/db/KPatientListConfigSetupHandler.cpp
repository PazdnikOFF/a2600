#include "db/KPatientListConfigSetupHandler.h"
#include "kernel/KConfig.h"
#include "sys/KSystem.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <mutex>
#include <memory>   // std::shared_ptr/unique_ptr (libstdc++ не тянет транзитивно)

namespace {
const char *SECTION = "ShowOnMainUi";   // реф. — единственная секция

// реф. ctor: создать каталог, при отсутствии файла создать ПУСТОЙ, затем KConfig.
std::shared_ptr<KConfig> makeConfig(const QString &path, const char *cls)
{
    const QString dir = QFileInfo(path).absolutePath();
    QDir d(dir);
    if (!d.exists())
        QDir().mkpath(d.absolutePath());

    if (!QFile::exists(path)) {
        QFile f(path);
        if (f.open(QIODevice::WriteOnly)) {   // реф. fopen(path,"w") — пустой файл
            f.close();
            qInfo() << cls << "create file success:" << path;
        } else {
            qWarning() << cls << "create file fail:" << path;
        }
    }
    return std::make_shared<KConfig>(path.toStdString());
}
} // namespace

KPatientListConfigSetupHandler::KPatientListConfigSetupHandler()
{
    const QString path = QDir(KSystem::ProtectedPath()).absoluteFilePath("patientsetup.ini");
    m_config = makeConfig(path, "KPatientListConfigSetupHandler");
}

KPatientListConfigSetupHandler *KPatientListConfigSetupHandler::GetInstance()
{
    // реф. std::call_once + m_instance.reset(new T); возвращает сырой указатель.
    static std::once_flag once;
    static std::shared_ptr<KPatientListConfigSetupHandler> inst;
    std::call_once(once, [] { inst.reset(new KPatientListConfigSetupHandler); });
    return inst.get();
}

// Все геттеры — ReadBool(section, key, true); сеттеры — WriteData (только в память).
bool KPatientListConfigSetupHandler::IsShowPatietID()
{
    return m_config->ReadBool(SECTION, "patientid", true);
}
void KPatientListConfigSetupHandler::SetIsShowPatietID(bool v)
{
    m_config->WriteData(SECTION, "patientid", v);
}

bool KPatientListConfigSetupHandler::IsShowApplicant()
{
    return m_config->ReadBool(SECTION, "applicant", true);
}
void KPatientListConfigSetupHandler::SetIsShowApplicant(bool v)
{
    m_config->WriteData(SECTION, "applicant", v);
}

bool KPatientListConfigSetupHandler::IsShowApplicantDate()
{
    return m_config->ReadBool(SECTION, "applicantdate", true);
}
void KPatientListConfigSetupHandler::SetIsShowApplicantDate(bool v)
{
    m_config->WriteData(SECTION, "applicantdate", v);
}

bool KPatientListConfigSetupHandler::IsShowBirthday()
{
    return m_config->ReadBool(SECTION, "birthday", true);
}
void KPatientListConfigSetupHandler::SetIsShowBirthday(bool v)
{
    m_config->WriteData(SECTION, "birthday", v);
}

bool KPatientListConfigSetupHandler::IsShowTelephone()
{
    return m_config->ReadBool(SECTION, "telephone", true);
}
void KPatientListConfigSetupHandler::SetIsShowTelephone(bool v)
{
    m_config->WriteData(SECTION, "telephone", v);
}

bool KPatientListConfigSetupHandler::IsShowSickbedNum()
{
    return m_config->ReadBool(SECTION, "bedno", true);
}
void KPatientListConfigSetupHandler::SetIsShowSickbedNum(bool v)
{
    m_config->WriteData(SECTION, "bedno", v);
}

bool KPatientListConfigSetupHandler::IsShowRegisterNumer()
{
    return m_config->ReadBool(SECTION, "registernumber", true);
}
void KPatientListConfigSetupHandler::SetIsShowRegisterNumer(bool v)
{
    m_config->WriteData(SECTION, "registernumber", v);
}

bool KPatientListConfigSetupHandler::IsShowSelfDefineField1()
{
    return m_config->ReadBool(SECTION, "userdefined1", true);
}
void KPatientListConfigSetupHandler::SetIsShowSelfDefineField1(bool v)
{
    m_config->WriteData(SECTION, "userdefined1", v);
}

bool KPatientListConfigSetupHandler::IsShowSelfDefineField2()
{
    return m_config->ReadBool(SECTION, "userdefined2", true);
}
void KPatientListConfigSetupHandler::SetIsShowSelfDefineField2(bool v)
{
    m_config->WriteData(SECTION, "userdefined2", v);
}

std::string KPatientListConfigSetupHandler::GetSelfDefineField1Title()
{
    return m_config->ReadString(SECTION, "userdefinedtitle1", std::string(""));
}
void KPatientListConfigSetupHandler::SetSelfDefineField1Title(const std::string &v)
{
    m_config->WriteData(SECTION, "userdefinedtitle1", v.c_str());
}

std::string KPatientListConfigSetupHandler::GetSelfDefineField2Title()
{
    return m_config->ReadString(SECTION, "userdefinedtitle2", std::string(""));
}
void KPatientListConfigSetupHandler::SetSelfDefineField2Title(const std::string &v)
{
    m_config->WriteData(SECTION, "userdefinedtitle2", v.c_str());
}

void KPatientListConfigSetupHandler::GetColumnIsShow(std::map<std::string, int> &out)
{
    // Реф.: ровно 7 записей, имена колонок ≠ ключам .ini (другое написание/регистр).
    out["PatientID"]       = IsShowPatietID();
    out["Applicants"]      = IsShowApplicant();
    out["ApplicantDate"]   = IsShowApplicantDate();
    out["PatientBirthday"] = IsShowBirthday();
    out["TelephoneNumber"] = IsShowTelephone();
    out["SickBedId"]       = IsShowSickbedNum();
    out["RegisterNumber"]  = IsShowRegisterNumer();
}

void KPatientListConfigSetupHandler::SaveConfig()
{
    m_config->Save();
}
