#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include <QHash>

// Конфигурация источника холодного света (LED-осветитель, реф. KColdLightConfig,
// g_pColdLightConfig, X-2600). Два конфига в system/coldlight/:
//   • per-продукт <SERIES>/<MODEL>/coldlight.ini — секция [V01]: VLSConfigNum +
//     VLSConfig0..N (комбинации спектральных режимов WL/EWL/SFI/VIST для кнопки VLS);
//   • coldlightCommPara.ini — [V01]: "<lightModel>\<mode>" → 26 float (интенсивности
//     LED-каналов и коэффициенты спектрального режима).
// Реф. методы: GetVLSConfigDisplayList, SetUserVLSConfig, ColdlightConfigPath.
class KColdLightConfig
{
public:
    static KColdLightConfig &GetInstance();   // реф. g_pColdLightConfig

    // --- VLS-конфиги (per-продукт coldlight.ini) ---
    bool LoadVLSConfig(const QString &coldlightIniPath);
    int  VLSConfigNum() const { return vlsConfigs_.size(); }
    // Список комбинаций режимов (реф. GetVLSConfigDisplayList); NULL-слоты убраны.
    QVector<QStringList> GetVLSConfigDisplayList() const { return vlsConfigs_; }

    // Выбор пользователя (реф. SetUserVLSConfig(configIdx, modeIdx)).
    void SetUserVLSConfig(int configIdx, int modeIdx);
    int  UserConfigIdx() const { return userConfig_; }
    int  UserModeIdx() const   { return userMode_; }
    QString CurrentMode() const;   // текущий режим из выбранного combo

    // --- LED-параметры спектральных режимов (coldlightCommPara.ini) ---
    bool LoadCommPara(const QString &commParaIniPath);
    // 26 float для источника <lightModel> в режиме <mode> (WL/EWL/SFI/VIST).
    QVector<float> GetLightParam(const QString &lightModel, const QString &mode) const;

private:
    KColdLightConfig() = default;
    QVector<QStringList> vlsConfigs_;                 // VLSConfig0..N
    QHash<QString, QVector<float>> commPara_;         // "model/mode" → 26 float
    int userConfig_ = 0;
    int userMode_ = 0;
};
