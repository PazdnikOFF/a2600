#include "ctrl/KColdLightConfig.h"

#include <QSettings>

KColdLightConfig &KColdLightConfig::GetInstance()
{
    static KColdLightConfig inst;   // реф. g_pColdLightConfig
    return inst;
}

bool KColdLightConfig::LoadVLSConfig(const QString &coldlightIniPath)
{
    vlsConfigs_.clear();
    QSettings ini(coldlightIniPath, QSettings::IniFormat);
    const int num = ini.value("V01/VLSConfigNum", 0).toInt();
    if (num <= 0)
        return false;
    for (int i = 0; i < num; ++i) {
        // VLSConfig<i>=WL,SFI,VIST — QSettings сам разбивает по запятой (toStringList).
        QStringList modes;
        for (const QString &m : ini.value(QString("V01/VLSConfig%1").arg(i)).toStringList()) {
            const QString t = m.trimmed();
            if (!t.isEmpty() && t != QLatin1String("NULL"))
                modes << t;
        }
        vlsConfigs_.append(modes);
    }
    return !vlsConfigs_.isEmpty();
}

void KColdLightConfig::SetUserVLSConfig(int configIdx, int modeIdx)
{
    // Реф. SetUserVLSConfig: зафиксировать выбранную комбинацию и режим в ней.
    if (configIdx < 0 || configIdx >= vlsConfigs_.size())
        return;
    userConfig_ = configIdx;
    const int n = vlsConfigs_[configIdx].size();
    userMode_ = (n > 0) ? qBound(0, modeIdx, n - 1) : 0;
}

QString KColdLightConfig::CurrentMode() const
{
    if (userConfig_ < 0 || userConfig_ >= vlsConfigs_.size())
        return QString();
    const QStringList &combo = vlsConfigs_[userConfig_];
    if (userMode_ < 0 || userMode_ >= combo.size())
        return QString();
    return combo[userMode_];
}

bool KColdLightConfig::LoadCommPara(const QString &commParaIniPath)
{
    commPara_.clear();
    QSettings ini(commParaIniPath, QSettings::IniFormat);
    ini.beginGroup("V01");
    // Ключи "<model>\<mode>" → QSettings трактует '\' как разделитель групп:
    // childGroups() = модели, внутри childKeys() = режимы; значение — список 26 float.
    for (const QString &model : ini.childGroups()) {
        ini.beginGroup(model);
        for (const QString &mode : ini.childKeys()) {
            QVector<float> vals;
            for (const QString &tok : ini.value(mode).toStringList()) {
                bool ok = false; const float v = tok.trimmed().toFloat(&ok);
                if (ok) vals.append(v);
            }
            if (!vals.isEmpty())
                commPara_.insert(model + "/" + mode, vals);
        }
        ini.endGroup();
    }
    ini.endGroup();
    return !commPara_.isEmpty();
}

QVector<float> KColdLightConfig::GetLightParam(const QString &lightModel,
                                               const QString &mode) const
{
    return commPara_.value(lightModel + "/" + mode);
}
