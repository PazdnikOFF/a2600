#include "sys/languageConfig.h"
#include "sys/KSystem.h"

#include <QSettings>
#include <QDir>

languageConfig &languageConfig::getInstance()
{
    static languageConfig inst;
    return inst;
}

QString languageConfig::cfgFile(const QString &given) const
{
    if (!given.isEmpty())
        return given;
    // Реф. путь: system/platform/mutilanguageinfo.ini.
    return QDir(KSystem::SystemPath()).absoluteFilePath("platform/mutilanguageinfo.ini");
}

void languageConfig::Init(const QString &iniPath)
{
    QSettings ini(cfgFile(iniPath), QSettings::IniFormat);
    // [MutiLanguageInfo] LanguageType/CurrentLanguage — типы языка (int-энум).
    languageType_    = ini.value("MutiLanguageInfo/LanguageType",    KLangChinese).toInt();
    currentLanguage_ = ini.value("MutiLanguageInfo/CurrentLanguage", KLangChinese).toInt();
    // [GooglePath] path/tabpath — каталог платформы и путь к kchinesePunct.tab.
    googlePath_ = ini.value("GooglePath/path").toString();
    puctPath_   = ini.value("GooglePath/tabpath").toString();
}

void languageConfig::setLanguageType(_KLanguageType t)
{
    // Реф.: stp w1,w1,[x0,#0x10] — значение пишется в оба поля.
    currentLanguage_ = t;
    languageType_    = t;
}

void languageConfig::setCurrentLanguage(_KLanguageType t)
{
    // Реф.: сменить currentLanguage_ только если t==Chinese(1) или t==languageType_.
    if (t == KLangChinese || t == languageType_)
        currentLanguage_ = t;
}
