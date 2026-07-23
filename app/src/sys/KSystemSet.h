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
    // Реф. GetSystemLanguage @0x65e5c8 / SetSystemLanguage @0x65fe88 — это ЦЕЛОЕ
    // (enum), хранимое под тем же ключом Common/Language, а НЕ строка:
    //   0=Chinese 1=English 2=Spanish 3=Italian 4=French 5=Russian 6=German 7=Polish
    // ⚠️ Геттер КЛАМПИТ: если значение < 0 или >= KProjectSet::LanguageMode(),
    // возвращается 1 (English) как запасной вариант.
    int  GetSystemLanguage() const;         void SetSystemLanguage(int v);
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

    // --- «Защищённый» system.ini (реф. GetProtectedSysIniPath @0x6582d0) ---
    // ⚠️ ЭТО ДРУГОЙ ФАЙЛ, не тот, что читают геттеры выше: KSystem::ProtectedPath() +
    // "system.ini" (в реф. — конкатенация, ProtectedPath оканчивается на "/"), т.е.
    // /home/root/data/protected/system.ini. Каталог создаётся при первом обращении
    // (QDir::mkpath, если не существует). Здесь живут продуктовая идентичность и
    // лицензия — то, что переживает сброс настроек.
    QString GetProtectedSysIniPath() const;
    void SetProtectedSysIniFile(const QString &path) { protectedFile_ = path; }  // не из реф.: для self-test
    QVariant ReadProtectedValue(const QString &key, const QVariant &def) const;   // реф. @0x658e80
    void     WriteProtectedValue(const QString &key, const QVariant &value);      // реф. @0x658da8

    // Лицензия/авторизация прибора. ⚠️ Все три ключа — с опечатками вендора, оставлены
    // как есть: System/ProductLastVaildDate (sic), System/ProductAuthbinMD5 (sic).
    // ⚠️ RemainDays в реф. — СТРОКА (Get/SetRemainDays(QString)), а не число; вызывающие
    // сами делают toInt(). Дефолт всех — пустая строка.
    QString GetProductCN() const;            void SetProductCN(const QString &v);
    QString GetLastValidDate() const;        void SetLastValidDate(const QString &v);
    QString GetRemainDays() const;           void SetRemainDays(const QString &v);
    QString GetAuthBinMD5() const;           void SetAuthBinMD5(const QString &v);
    // ⚠️ Флаг авторизации ПИШЕТСЯ как int, а ЧИТАЕТСЯ как строка (реф. GetProductAuthFlag
    // @0x65f0b8 отдаёт QString с тем же дефолтом ""), поэтому подписи асимметричны.
    QString GetProductAuthFlag() const;      void SetProductAuthFlag(int v);

    // Продуктовая идентичность. Все три читаются из защищённого system.ini и
    // ВАЛИДИРУЮТСЯ по спискам из project.ini: если сохранённого значения нет в списке,
    // возвращается ПЕРВЫЙ элемент списка (реф. `list.begin()`, не last).
    QString GetProductSeries() const;        // ключ Product/ProductSerise (sic!)
    QString GetProductModel() const;         // ключ Product/ProductModel, список — по серии
    QString GetProductRelaseVersion() const; // sic; ключ Product/ReleaseVersion, дефолт "V2.0"
    void SetProductSeries(const QString &v);
    void SetProductModel(const QString &v);
    void SetProductReleaseVersion(const QString &v);

private:
    KSystemSet() = default;
    QString cfgFile() const;
    QVariant read(const QString &key, const QVariant &def) const;
    void     write(const QString &key, const QVariant &value);
    QString cfgFile_;
    QString protectedFile_;   // не из реф.: подмена защищённого system.ini в self-test
};
