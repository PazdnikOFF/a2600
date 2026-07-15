#pragma once

#include <QString>
#include <QStringList>

// Конфиг OSD-меню эндоскопа (реф. KUserOsdSet, X-2600). Читает/пишет osd.ini
// (KSystem::UserPresetPath) — часть, НЕ покрытая KUserSet (тот держит video-параметры).
// Здесь: назначение функций на физические кнопки ручки эндоскопа (ButtomM/A/B ×
// Short/LongPress → ID функции), ножной переключатель (FootSwitch), режимы Operation/Iris.
//
// Список функций (реф. GetFunctionNameList) — фикс. по ID:
//   0 TR_Frz, 1 TR_Zm1, 2 TR_IRIS1, 3 TR_AGC1, 4 TR_IEnh, 5 TR_Snp,
//   6 TR_Brtnss+, 7 TR_Brtnss-, 8 TR_Ctrst, 9 TR_WBalance, 10 TR_LMode, 11 TR_Rcd.
class KUserOsdSet
{
public:
    static KUserOsdSet &GetInstance();

    // Функция, назначаемая на кнопку (ID = индекс в списке функций).
    enum Function {
        FUNC_FREEZE = 0, FUNC_ZOOM = 1, FUNC_IRIS = 2, FUNC_AGC = 3,
        FUNC_IMG_ENHANCE = 4, FUNC_SNAPSHOT = 5, FUNC_BRIGHT_UP = 6,
        FUNC_BRIGHT_DOWN = 7, FUNC_CONTRAST = 8, FUNC_WBALANCE = 9,
        FUNC_LIGHT_MODE = 10, FUNC_RECORD = 11, FUNC_COUNT = 12,
    };
    // Физическая кнопка ручки.
    enum Button { BTN_M, BTN_A, BTN_B };
    enum Press  { PRESS_SHORT, PRESS_LONG };

    void SetConfigFile(const QString &path) { cfgFile_ = path; }
    QString GetUserOsdConfigFile() const;              // osd.ini

    // Список TR-ключей функций по ID (реф. GetFunctionNameList — без фильтра enable).
    static QStringList GetFunctionNameList();
    // TR-ключ функции по ID ("" вне диапазона).
    static QString GetFunctionName(int funcId);

    // Кнопка → ID функции (реф. GetButtonFunctionId): [Buttom<X>]/<Short|Long>Press.
    int  GetButtonFunctionId(Button btn, Press press) const;
    void SaveButtonId(Button btn, Press press, int funcId) const;  // реф. SaveButtonId

    // Ножной переключатель [FootSwitch]/Switch1|Switch2 → ID функции.
    int  GetFootSwitchFunctionId(int idx /*1|2*/) const;
    void SaveFootSwitch(int idx, int funcId) const;

    // Режимы (реф. SaveOperationMode/SaveIrisMode).
    int  GetOperationMode() const;                     // [Operation]/Mode
    void SaveOperationMode(int mode) const;
    int  GetIrisMode() const;                          // [Iris]/Mode
    void SaveIrisMode(int mode) const;

private:
    KUserOsdSet() = default;
    QString cfgFile() const;                           // резолв пути
    QString cfgFile_;
    static QString buttonSection(Button btn);
    static QString pressKey(Press press);
};
