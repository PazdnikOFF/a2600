#include "sys/KSystemSet.h"

#include "sys/KProjectSet.h"
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

// --- [Manu] (реф. делегаты KManuPwdMng). Подтверждён ключ Manu/enable; прочие
// имена (leftTime/markTime/licenseKey) из бинарника не декодированы — по смыслу. ---
bool KSystemSet::GetManuEnable() const   { return read("Manu/enable", false).toBool(); }
void KSystemSet::SetManuEnable(bool v)   { write("Manu/enable", v); }

int  KSystemSet::GetManuLeftTime() const { return read("Manu/leftTime", 0).toInt(); }
void KSystemSet::SetManuLeftTime(int d)  { write("Manu/leftTime", d); }

QDate KSystemSet::GetManuMarkTime() const
{
    return QDate::fromString(read("Manu/markTime", "").toString(), "yyyy-MM-dd");
}
void KSystemSet::SetManuMarkTime(const QDate &d)
{
    write("Manu/markTime", d.toString("yyyy-MM-dd"));
}

QList<int> KSystemSet::GetManuLicenseKeyList() const
{
    QList<int> out;
    const QString s = read("Manu/licenseKey", "").toString();
    for (const QString &tok : s.split(',', Qt::SkipEmptyParts))
        out.append(tok.toInt());
    return out;
}
void KSystemSet::SetManuLicenseKey(int code)
{
    // Антиповтор: добавляем код, если его ещё нет (реф. — реестр использованных ключей).
    QList<int> list = GetManuLicenseKeyList();
    if (!list.contains(code))
        list.append(code);
    QStringList parts;
    for (int c : list)
        parts << QString::number(c);
    write("Manu/licenseKey", parts.join(','));
}

QString KSystemSet::GetProcessorSN() const { return read("Common/ProcessorSN", "").toString(); }

int KSystemSet::GetSystemLanguage() const
{
    // Реф.: сырое целое из Common/Language с клампом в 1 (English).
    bool ok = false;
    const int v = read("Common/Language", 1).toInt(&ok);
    if (!ok)
        return 1;
    const int mode = KProjectSet::GetInstance().LanguageMode();
    if (v < 0 || (mode > 0 && v >= mode))
        return 1;
    return v;
}

void KSystemSet::SetSystemLanguage(int v)
{
    write("Common/Language", v);
}
