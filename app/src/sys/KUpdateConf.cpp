#include "sys/KUpdateConf.h"
#include "sys/KSystem.h"

#include <QSettings>
#include <QDir>

KUpdateConf &KUpdateConf::GetInstance()
{
    static KUpdateConf inst;
    return inst;
}

QString KUpdateConf::cfgFile() const
{
    if (!cfgFile_.isEmpty())
        return cfgFile_;
    // реф.: SystemPath() + "presetdata/syspreset/matchedversion.ini".
    return QDir(KSystem::SystemPath())
        .absoluteFilePath("presetdata/syspreset/matchedversion.ini");
}

QStringList KUpdateConf::GetMatchedVersion(const QString &component) const
{
    // реф.: value("MatchedVersion/<component>").toStringList() (запятая → список).
    QSettings ini(cfgFile(), QSettings::IniFormat);
    QStringList out;
    for (const QString &v : ini.value("MatchedVersion/" + component).toStringList())
        if (!v.trimmed().isEmpty())
            out << v.trimmed();
    return out;
}

int KUpdateConf::MatchedNum() const
{
    QSettings ini(cfgFile(), QSettings::IniFormat);
    return ini.value("MatchedVersion/NUM", 0).toInt();
}

bool KUpdateConf::IsVersionMatched(const QString &component,
                                   const QString &installedVersion) const
{
    return GetMatchedVersion(component).contains(installedVersion.trimmed());
}
