#pragma once

#include <QString>

// Установленные версии компонентов прошивки (реф. KVersionConfig, X-2600).
// Читает /home/root/data/protected/version.ini, секция [Version]: <компонент>=<версия>
// (kernel/hmi/panel/camera/lcd/pap*…). В паре с KUpdateConf (matchedversion.ini)
// даёт проверку совместимости прошивки апдейт-пайплайном.
class KVersionConfig
{
public:
    static KVersionConfig &GetInstance();

    void SetConfigFile(const QString &path) { cfgFile_ = path; }

    // Установленная версия компонента (реф. GetVersion → "Version/<component>").
    QString GetVersion(const QString &component) const;
    void    SetVersion(const QString &component, const QString &version);  // реф. SetVersion
    // Полная строка версии (реф. GetCompleteVersion) — ключ Version/complete.
    QString GetCompleteVersion() const;
    QString GetKernelVersion() const { return GetVersion("kernel"); }
    QString GetAppSoftwareVersion() const { return GetVersion("pap"); }

    // Совместимость установленной версии компонента с matched (KUpdateConf).
    bool IsComponentCompatible(const QString &component) const;
    // Все ли перечисленные компоненты совместимы.
    bool IsCompatible(const QStringList &components) const;

private:
    KVersionConfig() = default;
    QString cfgFile() const;
    QString cfgFile_;
};
