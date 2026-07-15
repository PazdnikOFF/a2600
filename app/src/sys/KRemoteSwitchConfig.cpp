#include "sys/KRemoteSwitchConfig.h"
#include "sys/KSystem.h"

#include <QSettings>
#include <QDir>

KRemoteSwitchConfig &KRemoteSwitchConfig::GetInstance()
{
    static KRemoteSwitchConfig inst;
    return inst;
}

QString KRemoteSwitchConfig::ConfigFile() const
{
    if (!cfgFile_.isEmpty())
        return cfgFile_;
    return QDir(KSystem::UserPresetPath()).absoluteFilePath("user.ini");
}

int KRemoteSwitchConfig::readInt(const QString &section, const QString &key) const
{
    QSettings ini(ConfigFile(), QSettings::IniFormat);
    return ini.value(section + "/" + key, -1).toInt();
}

int KRemoteSwitchConfig::GetRemoteSwitchFunctionId(int idx) const
{
    return readInt("RemoteSwitch", QString("Switch%1").arg(idx));
}

int KRemoteSwitchConfig::GetFootSwitchFunctionId(int idx) const
{
    return readInt("FootSwitch", QString("Switch%1").arg(idx));
}

int KRemoteSwitchConfig::GetIHbMode(int idx) const
{
    return readInt("Enhance", QString("IHbMode%1").arg(idx));
}
