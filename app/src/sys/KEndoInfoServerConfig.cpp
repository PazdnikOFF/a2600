#include "sys/KEndoInfoServerConfig.h"
#include "sys/KSystem.h"

#include <QSettings>
#include <QDir>
#include <QFileInfo>

KEndoInfoServerConfig &KEndoInfoServerConfig::GetInstance()
{
    static KEndoInfoServerConfig inst;
    return inst;
}

QString KEndoInfoServerConfig::cfgDir() const
{
    return QDir(KSystem::SystemPath()).absoluteFilePath("presetdata/endoinfoserver");
}

QString KEndoInfoServerConfig::ConfigFile() const
{
    if (!cfgFile_.isEmpty())
        return cfgFile_;
    return QDir(cfgDir()).absoluteFilePath("endoinfoserver.ini");
}

QString KEndoInfoServerConfig::PublicKeyFile() const
{
    // Ключ лежит рядом с конфигом (реф. путь public_key.pem в той же папке).
    return QDir(QFileInfo(ConfigFile()).absolutePath()).absoluteFilePath("public_key.pem");
}

QString KEndoInfoServerConfig::value(const QString &key) const
{
    QSettings ini(ConfigFile(), QSettings::IniFormat);
    return ini.value("endoinfoserver/" + key).toString();
}

QString KEndoInfoServerConfig::Dns1() const            { return value("dns1"); }
QString KEndoInfoServerConfig::Dns2() const            { return value("dns2"); }
QString KEndoInfoServerConfig::Proxy() const           { return value("proxy"); }
QString KEndoInfoServerConfig::LoginUrl() const        { return value("loginurl"); }
QString KEndoInfoServerConfig::EndoInfoPostUrl() const { return value("endoinfoposturl"); }

bool KEndoInfoServerConfig::IsValid() const
{
    return !EndoInfoPostUrl().isEmpty();
}
