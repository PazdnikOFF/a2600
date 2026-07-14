#include "sys/KVersionConfig.h"
#include "sys/KUpdateConf.h"
#include "sys/KSystem.h"

#include <QSettings>
#include <QDir>
#include <QStringList>

KVersionConfig &KVersionConfig::GetInstance()
{
    static KVersionConfig inst;
    return inst;
}

QString KVersionConfig::cfgFile() const
{
    if (!cfgFile_.isEmpty())
        return cfgFile_;
    // реф.: /home/root/data/protected/version.ini
    return QDir(KSystem::DataPath()).absoluteFilePath("protected/version.ini");
}

QString KVersionConfig::GetVersion(const QString &component) const
{
    // реф.: value(QString("Version/%1").arg(component)).toString().
    QSettings ini(cfgFile(), QSettings::IniFormat);
    return ini.value(QString("Version/%1").arg(component)).toString();
}

void KVersionConfig::SetVersion(const QString &component, const QString &version)
{
    QSettings ini(cfgFile(), QSettings::IniFormat);
    ini.setValue(QString("Version/%1").arg(component), version);
}

QString KVersionConfig::GetCompleteVersion() const
{
    QSettings ini(cfgFile(), QSettings::IniFormat);
    return ini.value("Version/complete").toString();
}

bool KVersionConfig::IsComponentCompatible(const QString &component) const
{
    const QString installed = GetVersion(component);
    if (installed.isEmpty())
        return false;
    return KUpdateConf::GetInstance().IsVersionMatched(component, installed);
}

bool KVersionConfig::IsCompatible(const QStringList &components) const
{
    for (const QString &c : components)
        if (!IsComponentCompatible(c))
            return false;
    return true;
}
