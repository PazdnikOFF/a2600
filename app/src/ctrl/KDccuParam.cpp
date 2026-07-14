#include "ctrl/KDccuParam.h"
#include "sys/KSystem.h"

#include <QDir>
#include <QSettings>

KDccuParam &KDccuParam::GetInstance()
{
    static KDccuParam inst;   // реф. синглтон
    return inst;
}

QString KDccuParam::ConfigFile() const
{
    // Реф.: SystemPath() + "presetdata/syspreset/dccuparam.ini".
    return QDir(KSystem::SystemPath())
        .absoluteFilePath("presetdata/syspreset/dccuparam.ini");
}

QVariant KDccuParam::ReadDccuParam(const QString &key, const QVariant &def) const
{
    QSettings ini(ConfigFile(), QSettings::IniFormat);
    return ini.value(key, def);
}

void KDccuParam::WriteDccuParam(const QString &key, const QVariant &value)
{
    QSettings ini(ConfigFile(), QSettings::IniFormat);
    ini.setValue(key, value);
}
