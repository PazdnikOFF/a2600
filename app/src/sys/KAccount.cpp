#include "sys/KAccount.h"
#include "sys/KSystem.h"

#include <QCryptographicHash>
#include <QSettings>
#include <QDir>
#include <QRegExp>

KAccount &KAccount::GetInstance()
{
    static KAccount inst;   // реф. синглтон
    return inst;
}

QString KAccount::cfgFile() const
{
    if (!cfgFile_.isEmpty())
        return cfgFile_;
    return QDir(KSystem::SystemPath()).absoluteFilePath("system.ini");
}

QString KAccount::ConvertPasswordToMD5(const QString &password)
{
    // реф. base::MD5String → 32 hex-символа в нижнем регистре.
    return QString::fromLatin1(
        QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Md5).toHex());
}

QString KAccount::GetAdminPassWord() const
{
    QSettings ini(cfgFile(), QSettings::IniFormat);
    return ini.value("Account/AdminPassword", GetAdminDefaultPassWord()).toString();
}

bool KAccount::SaveAdminPassWord(const QString &plainPassword)
{
    if (!ValidateIfPWValid(plainPassword))
        return false;
    QSettings ini(cfgFile(), QSettings::IniFormat);
    ini.setValue("Account/AdminPassword", ConvertPasswordToMD5(plainPassword));
    return true;
}

bool KAccount::IsAdminPasswordChange() const
{
    return GetAdminPassWord() != GetAdminDefaultPassWord();
}

QString KAccount::GetPasswordRegExp()
{
    // реф. GetPasswordRegExp: запрещён набор CJK/полноширинной пунктуации.
    static const QString kForbidden = QString::fromUtf8(
        "！『』（）；：“”‘’、……￥【】？《》，。");
    QString cls;
    for (const QChar &c : kForbidden)
        cls += QRegExp::escape(QString(c));
    return QString("[^%1]+$").arg(cls);
}

bool KAccount::ValidateIfPWValid(const QString &password)
{
    if (password.isEmpty())
        return false;
    QRegExp re(GetPasswordRegExp());
    return re.exactMatch(password);
}

KAccount::Role KAccount::Login(const QString &user, const QString &plainPassword)
{
    // Заблокированный аккаунт не входит (реф. проверка LockStatus в начале Login).
    if (GetAccountLockStatus(user)) {
        role_ = RoleNone;
        return RoleNone;
    }

    // Роль по имени пользователя (реф. сравнение с "adm"/"manu"/admin).
    Role wantRole = RoleNone;
    if (user == GetAdmin() || user == QLatin1String("adm"))
        wantRole = RoleAdmin;
    else if (user == QLatin1String("manu"))
        wantRole = RoleManu;
    else {
        updateAccountLoginRes(user, false);
        role_ = RoleNone;
        return RoleNone;
    }

    // Проверка пароля (MD5). Для admin — сохранённый/дефолтный; manu — дефолт по имени.
    const QString expect = (wantRole == RoleAdmin)
        ? GetAdminPassWord()
        : ConvertPasswordToMD5(QLatin1String("manu"));
    const bool ok = ConvertPasswordToMD5(plainPassword) == expect;
    updateAccountLoginRes(user, ok);
    role_ = ok ? wantRole : RoleNone;
    return role_;
}

void KAccount::LogOut()
{
    role_ = RoleNone;
}

void KAccount::updateAccountLoginRes(const QString &user, bool success)
{
    // реф. UpdateAccountLoginRes: считаем неудачи подряд; при успехе — сброс.
    QSettings ini(cfgFile(), QSettings::IniFormat);
    const QString key = "Account/FailTimes_" + user;
    if (success) {
        ini.remove(key);
        ini.setValue("Account/LockStatus_" + user, false);
        return;
    }
    const int fails = ini.value(key, 0).toInt() + 1;
    ini.setValue(key, fails);
    if (fails >= kLockThreshold)
        ini.setValue("Account/LockStatus_" + user, true);
}

bool KAccount::GetAccountLockStatus(const QString &user) const
{
    QSettings ini(cfgFile(), QSettings::IniFormat);
    return ini.value("Account/LockStatus_" + user, false).toBool();
}

int KAccount::GetConstantLoginFailTimes(const QString &user) const
{
    QSettings ini(cfgFile(), QSettings::IniFormat);
    return ini.value("Account/FailTimes_" + user, 0).toInt();
}

void KAccount::ResetAccountLockInfo(const QString &user)
{
    QSettings ini(cfgFile(), QSettings::IniFormat);
    ini.remove("Account/FailTimes_" + user);
    ini.setValue("Account/LockStatus_" + user, false);
}
