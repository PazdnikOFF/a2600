#pragma once

#include <QString>

// Конфиг внешних переключателей и IHb-режимов (реф. чтение user.ini, X-2600).
// Файл: presetdata/userpreset/user.ini. Секции, не покрытые KUserSet/KUserOsdSet:
//   [RemoteSwitch] Switch1..4 — кнопки пульта ДУ → ID функции (как в KUserOsdSet);
//   [FootSwitch]   Switch1..2 — ножной переключатель → ID функции;
//   [Enhance]      IHbMode1..3 — сопоставление IHb-режимов (тон гемоглобина).
// ID функций совпадают со списком KUserOsdSet::GetFunctionName.
class KRemoteSwitchConfig
{
public:
    static KRemoteSwitchConfig &GetInstance();

    void SetConfigFile(const QString &path) { cfgFile_ = path; }
    QString ConfigFile() const;                    // …/userpreset/user.ini

    // Пульт ДУ: кнопка idx (1..4) → ID функции (реф. [RemoteSwitch]/Switch<idx>).
    int GetRemoteSwitchFunctionId(int idx) const;
    // Ножной переключатель idx (1..2) → ID функции ([FootSwitch]/Switch<idx>).
    int GetFootSwitchFunctionId(int idx) const;
    // IHb-режим idx (1..3) → значение ([Enhance]/IHbMode<idx>).
    int GetIHbMode(int idx) const;

private:
    KRemoteSwitchConfig() = default;
    int readInt(const QString &section, const QString &key) const;
    QString cfgFile_;
};
