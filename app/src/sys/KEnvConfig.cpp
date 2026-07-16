#include "sys/KEnvConfig.h"
#include "sys/KSystem.h"

KEnvConfig::KEnvConfig()
{
    // реф.: m_strBaseDir кэшируется один раз в ctor.
    m_strBaseDir = KSystem::DataPath().toUtf8().constData();
}

KEnvConfig &KEnvConfig::GetInstance()
{
    static KEnvConfig inst;   // реф. — function-local static (__cxa_guard)
    return inst;
}

std::string KEnvConfig::GetBaseDir() const
{
    return m_strBaseDir;
}

std::string KEnvConfig::GetReadOnlyBaseDir() const
{
    // реф. не кэширует — свежий вызов KSystem при каждом обращении.
    return KSystem::ProjectPresetPath().toUtf8().constData();
}

std::string KEnvConfig::GetUsrDir() const
{
    return KSystem::ProjectUserPresetPath().toUtf8().constData();
}
