#pragma once

#include <QDate>
#include <QList>
#include <QString>
#include <QVariant>

// Системные настройки (реф. KSystemSet, X-2600). QSettings-ini (system.ini),
// ключи из реверса: Common/Language, Common/DateFormat, Common/InputMethod,
// Account/autologin, Account/forcelogout, Account/forcelogoutTime + яркость/громкость.
class KSystemSet
{
public:
    static KSystemSet &GetInstance();

    void SetConfigFile(const QString &path) { cfgFile_ = path; }

    // Общие (реф. Common/*)
    QString Language() const;               void SetLanguage(const QString &v);
    QString DateFormat() const;             void SetDateFormat(const QString &v);
    QString InputMethod() const;            void SetInputMethod(const QString &v);

    // Экран/звук
    int  Brightness() const;                void SetBrightness(int v);
    int  Volume() const;                    void SetVolume(int v);

    // Вход/выход (реф. Account/*)
    bool AutoLogin() const;                 void SetAutoLogin(bool v);
    bool ForceLogout() const;               void SetForceLogout(bool v);
    int  ForceLogoutTime() const;           void SetForceLogoutTime(int minutes);

    // Контур доступа производителя (реф. [Manu], делегаты KManuPwdMng).
    // Подтверждён дизасмом ключ Manu/enable; остальные имена ключей в бинарнике
    // не декодированы (реф. эти геттеры/сеттеры живут в KSystemSet) — приняты по смыслу.
    bool GetManuEnable() const;             void SetManuEnable(bool v);
    int  GetManuLeftTime() const;           void SetManuLeftTime(int days);
    QDate GetManuMarkTime() const;          void SetManuMarkTime(const QDate &d);
    QList<int> GetManuLicenseKeyList() const;   void SetManuLicenseKey(int code); // добавляет в список

    // Серийник процессора (реф. GetProcessorSN — на устройстве из EEPROM; здесь из ini).
    QString GetProcessorSN() const;

private:
    KSystemSet() = default;
    QString cfgFile() const;
    QVariant read(const QString &key, const QVariant &def) const;
    void     write(const QString &key, const QVariant &value);
    QString cfgFile_;
};
