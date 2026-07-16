#include "db/KWorklistConfigSetupHandler.h"
#include "kernel/KConfig.h"
#include "sys/KSystem.h"

#include <QDate>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <mutex>

namespace {
const char *SECTION     = "ShowOnMainUi";   // реф. — единственная секция
const char *DATE_FORMAT = "yyyy-MM-dd";     // литерал реф.

// Реф. повторяет эту последовательность в ctor КАЖДОГО хендлера (см. также
// KPatientListConfigSetupHandler): каталог → пустой файл → KConfig.
std::shared_ptr<KConfig> makeConfig(const QString &path, const char *cls)
{
    const QString dir = QFileInfo(path).absolutePath();
    QDir d(dir);
    if (!d.exists())
        QDir().mkpath(d.absolutePath());

    if (!QFile::exists(path)) {
        QFile f(path);
        if (f.open(QIODevice::WriteOnly)) {
            f.close();
            qInfo() << cls << "create file success:" << path;
        } else {
            qWarning() << cls << "create file fail:" << path;
        }
    }
    return std::make_shared<KConfig>(path.toStdString());
}
} // namespace

KWorkListConfigSetupHandler::KWorkListConfigSetupHandler()
{
    const QString path = QDir(KSystem::ProtectedPath()).absoluteFilePath("worklistsetup.ini");
    m_config = makeConfig(path, "KWorkListConfigSetupHandler");
}

KWorkListConfigSetupHandler *KWorkListConfigSetupHandler::GetInstance()
{
    static std::once_flag once;
    static std::shared_ptr<KWorkListConfigSetupHandler> inst;
    std::call_once(once, [] { inst.reset(new KWorkListConfigSetupHandler); });
    return inst.get();
}

bool KWorkListConfigSetupHandler::IsShowPatientID()
{
    return m_config->ReadBool(SECTION, "IsPatientidOn", true);
}
void KWorkListConfigSetupHandler::SetIsShowPatientID(bool v)
{
    m_config->WriteData(SECTION, "IsPatientidOn", v);
}
std::string KWorkListConfigSetupHandler::GetShowPatientID()
{
    return m_config->ReadString(SECTION, "patientid", std::string(""));
}
void KWorkListConfigSetupHandler::SetShowPatientID(const std::string &v)
{
    m_config->WriteData(SECTION, "patientid", v.c_str());
}

bool KWorkListConfigSetupHandler::IsShowPatientName()
{
    return m_config->ReadBool(SECTION, "IsPatientnameOn", true);
}
void KWorkListConfigSetupHandler::SetIsShowPatientName(bool v)
{
    m_config->WriteData(SECTION, "IsPatientnameOn", v);
}
std::string KWorkListConfigSetupHandler::GetShowPatientName()
{
    return m_config->ReadString(SECTION, "patientname", std::string(""));
}
void KWorkListConfigSetupHandler::SetShowPatientName(const std::string &v)
{
    m_config->WriteData(SECTION, "patientname", v.c_str());
}

bool KWorkListConfigSetupHandler::IsShowRegisterNumber()
{
    return m_config->ReadBool(SECTION, "IsRegisterNumberOn", true);
}
void KWorkListConfigSetupHandler::SetIsShowRegisterNumber(bool v)
{
    m_config->WriteData(SECTION, "IsRegisterNumberOn", v);
}
std::string KWorkListConfigSetupHandler::GetShowRegisterNumber()
{
    return m_config->ReadString(SECTION, "RegisterNumber", std::string(""));
}
void KWorkListConfigSetupHandler::SetShowRegisterNumber(const std::string &v)
{
    m_config->WriteData(SECTION, "RegisterNumber", v.c_str());
}

bool KWorkListConfigSetupHandler::IsShowPlantime()
{
    return m_config->ReadBool(SECTION, "IsPlantimeOn", true);
}
void KWorkListConfigSetupHandler::SetIsShowPlantime(bool v)
{
    m_config->WriteData(SECTION, "IsPlantimeOn", v);
}

void KWorkListConfigSetupHandler::GetShowPlantime(QDate &start, QDate &end)
{
    // реф.: QDate::fromString(QString::fromUtf8(ReadString(...,"")), "yyyy-MM-dd")
    start = QDate::fromString(
        QString::fromStdString(m_config->ReadString(SECTION, "plantimestart", std::string(""))),
        DATE_FORMAT);
    end = QDate::fromString(
        QString::fromStdString(m_config->ReadString(SECTION, "plantimeend", std::string(""))),
        DATE_FORMAT);
}

void KWorkListConfigSetupHandler::SetShowPlantime(const QDate &start, const QDate &end)
{
    m_config->WriteData(SECTION, "plantimestart", start.toString(DATE_FORMAT).toUtf8().constData());
    m_config->WriteData(SECTION, "plantimeend", end.toString(DATE_FORMAT).toUtf8().constData());
}

bool KWorkListConfigSetupHandler::IsShowInspectEquipment()
{
    return m_config->ReadBool(SECTION, "IsEquipmentOn", true);
}
void KWorkListConfigSetupHandler::SetIsShowInspectEquipment(bool v)
{
    m_config->WriteData(SECTION, "IsEquipmentOn", v);
}
int KWorkListConfigSetupHandler::GetShowInspectEquipment()
{
    return m_config->ReadInt(SECTION, "Equipment", 0);
}
void KWorkListConfigSetupHandler::SetShowInspectEquipment(const int &v)
{
    m_config->WriteData(SECTION, "Equipment", v);
}

void KWorkListConfigSetupHandler::SaveConfig()
{
    m_config->Save();
}
