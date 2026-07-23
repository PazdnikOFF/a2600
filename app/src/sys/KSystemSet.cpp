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

// ============================================================================
// «Защищённый» system.ini: продуктовая идентичность + лицензия.
// Реф. KSystemSet::GetProtectedSysIniPath @0x6582d0, Read/WriteProtectedValue
// @0x658e80 / @0x658da8. Отдельный файл от обычных настроек — переживает сброс.
// ============================================================================

QString KSystemSet::GetProtectedSysIniPath() const
{
    if (!protectedFile_.isEmpty())
        return protectedFile_;
    // Реф.: QDir dir(ProtectedPath()); if (!dir.exists()) dir.mkpath(ProtectedPath());
    const QString dir = KSystem::ProtectedPath();
    QDir d(dir);
    if (!d.exists())
        d.mkpath(dir);
    return d.absoluteFilePath("system.ini");
}

QVariant KSystemSet::ReadProtectedValue(const QString &key, const QVariant &def) const
{
    QSettings ini(GetProtectedSysIniPath(), QSettings::IniFormat);
    return ini.value(key, def);
}

void KSystemSet::WriteProtectedValue(const QString &key, const QVariant &value)
{
    QSettings ini(GetProtectedSysIniPath(), QSettings::IniFormat);
    ini.setValue(key, value);
}

// --- Лицензия. Дефолт у всех — пустая строка (реф. QVariant("")). ---

QString KSystemSet::GetProductCN() const
{ return ReadProtectedValue("System/ProductCN", "").toString(); }
void KSystemSet::SetProductCN(const QString &v)
{ WriteProtectedValue("System/ProductCN", v); }

// ⚠️ Ключ с опечаткой вендора: «Vaild» вместо «Valid» — оставлен как в прошивке.
QString KSystemSet::GetLastValidDate() const
{ return ReadProtectedValue("System/ProductLastVaildDate", "").toString(); }
void KSystemSet::SetLastValidDate(const QString &v)
{ WriteProtectedValue("System/ProductLastVaildDate", v); }

QString KSystemSet::GetRemainDays() const
{ return ReadProtectedValue("System/ProductRemainDays", "").toString(); }
void KSystemSet::SetRemainDays(const QString &v)
{ WriteProtectedValue("System/ProductRemainDays", v); }

// ⚠️ Асимметрия из реф.: пишется int, читается строкой.
QString KSystemSet::GetProductAuthFlag() const
{ return ReadProtectedValue("System/ProductAuthFlag", "").toString(); }
void KSystemSet::SetProductAuthFlag(int v)
{ WriteProtectedValue("System/ProductAuthFlag", v); }

// ⚠️ И здесь опечатка вендора: «Authbin» строчной b.
QString KSystemSet::GetAuthBinMD5() const
{ return ReadProtectedValue("System/ProductAuthbinMD5", "").toString(); }
void KSystemSet::SetAuthBinMD5(const QString &v)
{ WriteProtectedValue("System/ProductAuthbinMD5", v); }

// --- Продуктовая идентичность (с валидацией по спискам project.ini). ---

QString KSystemSet::GetProductSeries() const
{
    // ⚠️ Ключ «ProductSerise» — опечатка вендора.
    QString v = ReadProtectedValue("Product/ProductSerise", "").toString();
    const QStringList list = KProjectSet::GetInstance().GetProductSeriesList();
    // Реф.: если значения нет в списке — берётся ПЕРВЫЙ элемент (`array + begin*8`),
    // а не последний. При пустом списке реф. читает мусор — у нас пустая строка.
    if (!list.contains(v, Qt::CaseSensitive))
        v = list.value(0);
    return v;
}

QString KSystemSet::GetProductModel() const
{
    QString v = ReadProtectedValue("Product/ProductModel", "").toString();
    // Реф. вызывает GetProductSeries() БЕЗУСЛОВНО — список моделей всегда берётся
    // для текущей серии, даже если сохранённая модель валидна.
    const QStringList list = KProjectSet::GetInstance().GetProductModelList(GetProductSeries());
    if (!list.contains(v, Qt::CaseSensitive))
        v = list.value(0);
    return v;
}

QString KSystemSet::GetProductRelaseVersion() const
{
    QString v = ReadProtectedValue("Product/ReleaseVersion", "V2.0").toString();
    const QStringList list = KProjectSet::GetInstance().GetReleaseVersionList();
    if (!list.contains(v, Qt::CaseSensitive))
        v = list.value(0);
    // ⚠️ Реф. затирает результат на "V1.0", если IsFirstRegisterVersion() — но та
    // в прошивке жёстко возвращает false (тело: `mov w0,#0; ret`), т.е. ветка мёртвая.
    if (KProjectSet::GetInstance().IsFirstRegisterVersion())
        v = QStringLiteral("V1.0");
    return v;
}

void KSystemSet::SetProductSeries(const QString &v)
{ WriteProtectedValue("Product/ProductSerise", v); }
void KSystemSet::SetProductModel(const QString &v)
{ WriteProtectedValue("Product/ProductModel", v); }
void KSystemSet::SetProductReleaseVersion(const QString &v)
{ WriteProtectedValue("Product/ReleaseVersion", v); }
