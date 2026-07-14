#include "sys/KSystemSet.h"
#include "sys/KSystem.h"

#include <QSettings>
#include <QDir>
#include <QVariant>

KSystemSet &KSystemSet::GetInstance()
{
    static KSystemSet inst;
    return inst;
}

QString KSystemSet::cfgFile() const
{
    if (!cfgFile_.isEmpty())
        return cfgFile_;
    return QDir(KSystem::SystemPath()).absoluteFilePath("system.ini");
}

QVariant KSystemSet::read(const QString &key, const QVariant &def) const
{
    QSettings ini(cfgFile(), QSettings::IniFormat);
    return ini.value(key, def);
}

void KSystemSet::write(const QString &key, const QVariant &value)
{
    QSettings ini(cfgFile(), QSettings::IniFormat);
    ini.setValue(key, value);
}

QString KSystemSet::Language() const    { return read("Common/Language", "English").toString(); }
void KSystemSet::SetLanguage(const QString &v)    { write("Common/Language", v); }

QString KSystemSet::DateFormat() const  { return read("Common/DateFormat", "yyyy-MM-dd").toString(); }
void KSystemSet::SetDateFormat(const QString &v)  { write("Common/DateFormat", v); }

QString KSystemSet::InputMethod() const { return read("Common/InputMethod", "").toString(); }
void KSystemSet::SetInputMethod(const QString &v) { write("Common/InputMethod", v); }

int  KSystemSet::Brightness() const     { return read("Common/Brightness", 80).toInt(); }
void KSystemSet::SetBrightness(int v)   { write("Common/Brightness", v); }

int  KSystemSet::Volume() const         { return read("Common/Volume", 50).toInt(); }
void KSystemSet::SetVolume(int v)       { write("Common/Volume", v); }

bool KSystemSet::AutoLogin() const      { return read("Account/autologin", false).toBool(); }
void KSystemSet::SetAutoLogin(bool v)   { write("Account/autologin", v); }

bool KSystemSet::ForceLogout() const    { return read("Account/forcelogout", false).toBool(); }
void KSystemSet::SetForceLogout(bool v) { write("Account/forcelogout", v); }

int  KSystemSet::ForceLogoutTime() const      { return read("Account/forcelogoutTime", 30).toInt(); }
void KSystemSet::SetForceLogoutTime(int m)    { write("Account/forcelogoutTime", m); }
