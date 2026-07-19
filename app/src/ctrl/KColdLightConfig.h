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
// Параметры автоматического диммирования (реф. _KAutomaticDimmerParam, sizeof 100/0x64).
// Значение в ini — 26 токенов через запятую; отображение «токен → поле» СВЕРЕНО дизасмом.
// ВНИМАНИЕ: ИМЕНА ПОЛЕЙ В ОРИГИНАЛЕ НЕВОССТАНОВИМЫ (нет строк .rodata / ассертов / форматов),
// поэтому поля названы по СМЕЩЕНИЮ — выдумывать семантику нельзя. Потребитель —
// K3ADimming::SetDimmingPidParam (чистое копирование полей, без логики).
// КВИРК РЕФ. (обязательно сохранить): хвост ПЕРЕСТАВЛЕН — токен 23 → 0x58, токен 24 → 0x5c,
// токен 25 → 0x54. Запись делает ту же перестановку, поэтому round-trip сходится.
struct _KAutomaticDimmerParam {
    float f00 = 0, f04 = 0, f08 = 0, f0c = 0, f10 = 0, f14 = 0;   // токены 0..5
    float f18 = 0, f1c = 0, f20 = 0, f24 = 0, f28 = 0, f2c = 0;   // токены 6..11
    float f30 = 0, f34 = 0, f38 = 0, f3c = 0, f40 = 0, f44 = 0;   // токены 12..17
    qint8  i48 = 0;    // токен 18 — ЗНАКОВЫЙ (в ini встречается -3), реф. toInt()
    qint8  i49 = 0;    // токен 19 — ЗНАКОВЫЙ, реф. toInt()
    quint8 u4a = 0;    // токен 20 — БЕЗЗНАКОВЫЙ, реф. toUInt()
    // 0x4b — padding
    float f4c = 0;     // токен 21
    float f50 = 0;     // токен 22
    float f54 = 0;     // токен 25 (перестановка!)
    float f58 = 0;     // токен 23 (перестановка!)
    float f5c = 0;     // токен 24 (перестановка!)
    quint32 pad60 = 0; // 0x60 — из ini не читается и в ini не пишется (только добивает sizeof)
};

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

    // --- Параметры авто-диммирования (реф. 4 метода, void; сверено дизасмом) ---
    // ОБЩАЯ СЕМАНТИКА обеих пар (в бинарнике их тела ПОБАЙТОВО СОВПАДАЮТ, различается
    // ТОЛЬКО имя файла): ключ = "V01/<endoModel>/<lightMode>" (QSettings пишет его на диск
    // как [V01] + "<endoModel>\<lightMode>"). Подстановки — ТОЛЬКО при ПУСТОМ аргументе:
    // endoModel=="" → "DefaultParam", lightMode=="" → "default"; фолбэка «ключ не найден»
    // НЕТ. В endoModel символ '/' заменяется на "__" (модель камеры может содержать '/').
    // Значение читается как QStringList; если элементов МЕНЬШЕ 26 — лог ошибки и ВЫХОД,
    // выходной параметр НЕ ТРОГАЕТСЯ.
    // Camera2A-пара → coldlightCamera2aPara.ini; Automatic-пара → coldlightCommPara.ini.
    void GetCameraManuDimmerParam(QString endoModel, QString lightMode,
                                  _KAutomaticDimmerParam &param) const;
    void SetCameraManuDimmerParam(QString endoModel, QString lightMode,
                                  _KAutomaticDimmerParam param) const;   // реф. — ПО ЗНАЧЕНИЮ
    void GetAutomaticDimmerParam(QString endoModel, QString lightMode,
                                 _KAutomaticDimmerParam &param) const;
    void SetAutomaticDimmerParam(QString endoModel, QString lightMode,
                                 _KAutomaticDimmerParam param) const;    // реф. — ПО ЗНАЧЕНИЮ

private:
    KColdLightConfig() = default;
    QVector<QStringList> vlsConfigs_;                 // VLSConfig0..N
    QHash<QString, QVector<float>> commPara_;         // "model/mode" → 26 float
    int userConfig_ = 0;
    int userMode_ = 0;
};
