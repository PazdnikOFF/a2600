#pragma once

#include <QString>

// Аккаунты и вход (реф. KAccount, X-2600). Роли: admin (обслуживание), manu
// (производитель). Пароли хранятся MD5-хэшем (реф. ConvertPasswordToMD5 →
// base::MD5String). Дефолтный пароль администратора = MD5(имя "admin").
// Блокировка после N неудачных попыток (реф. GetConstantLoginFailTimes/LockStatus).
// Настройки/хэши персистятся в QSettings-ini (ключи Account/*).
class KAccount
{
public:
    static KAccount &GetInstance();

    enum Role { RoleNone = 0, RoleAdmin, RoleManu };   // реф. CurrentRole

    // Путь ini аккаунтов (для тестов/диагностики); по умолчанию из KSystem.
    void SetConfigFile(const QString &path) { cfgFile_ = path; }

    // Имя администратора (реф. GetAdmin) — "admin".
    static QString GetAdmin() { return QStringLiteral("admin"); }

    // MD5-хэш пароля в hex (реф. ConvertPasswordToMD5 → base::MD5String).
    static QString ConvertPasswordToMD5(const QString &password);
    // Дефолтный пароль администратора = MD5(GetAdmin()) (реф. GetAdminDefaultPassWord).
    static QString GetAdminDefaultPassWord() { return ConvertPasswordToMD5(GetAdmin()); }

    // Текущий сохранённый пароль администратора (MD5); если не задан — дефолт.
    QString GetAdminPassWord() const;              // реф. GetAdminPassWord
    bool    SaveAdminPassWord(const QString &plainPassword);  // реф. SaveAdminPassWord (хэширует)
    bool    IsAdminPasswordChange() const;          // пароль отличается от дефолтного

    // Валидность пароля по политике (реф. ValidateIfPWValid/GetPasswordRegExp):
    // запрещён набор CJK-пунктуации; длина 1..N.
    static QString GetPasswordRegExp();
    static bool    ValidateIfPWValid(const QString &password);

    // Вход (реф. Login/UserLogin). Возвращает роль или RoleNone. Обновляет
    // счётчик неудач и блокировку. Заблокированный аккаунт входа не даёт.
    Role Login(const QString &user, const QString &plainPassword);
    void LogOut();                                  // реф. LogOut
    Role CurrentRole() const { return role_; }      // реф. CurrentRole

    // Блокировка (реф. GetAccountLockStatus/GetConstantLoginFailTimes/ResetAccountLockInfo).
    bool GetAccountLockStatus(const QString &user) const;
    int  GetConstantLoginFailTimes(const QString &user) const;
    void ResetAccountLockInfo(const QString &user);
    static int LockFailThreshold() { return kLockThreshold; }

private:
    KAccount() = default;
    QString cfgFile() const;
    void updateAccountLoginRes(const QString &user, bool success);  // реф. UpdateAccountLoginRes

    static constexpr int kLockThreshold = 5;   // блокировка после 5 неудач подряд
    QString cfgFile_;
    Role    role_ = RoleNone;
};
