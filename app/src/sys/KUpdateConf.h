#pragma once

#include <QString>
#include <QStringList>

// Матрица совместимости версий прошивки (реф. KUpdateConf, X-2600).
// Читает system/presetdata/syspreset/matchedversion.ini, секция [MatchedVersion]:
//   NUM=<число> + <компонент>=<верс1>[,<верс2>…] (допустимые версии компонента).
// Компоненты: kernel, hmi, panel, pap, pas, papp00..80, lcd, camera.
// Используется апдейт-пайплайном: установленная версия компонента (KVersionConfig)
// должна входить в список matched-версий, иначе — несовместимость.
class KUpdateConf
{
public:
    static KUpdateConf &GetInstance();

    void SetConfigFile(const QString &path) { cfgFile_ = path; }

    // Допустимые версии компонента (реф. GetMatchedVersion → toStringList).
    QStringList GetMatchedVersion(const QString &component) const;
    int  MatchedNum() const;                       // [MatchedVersion]NUM
    // Установленная версия входит в список допустимых (точное совпадение строки).
    bool IsVersionMatched(const QString &component, const QString &installedVersion) const;

private:
    KUpdateConf() = default;
    QString cfgFile() const;
    QString cfgFile_;
};
