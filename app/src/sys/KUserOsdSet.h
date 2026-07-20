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
    // ⚠️ ПОРЯДОК ЗНАЧЕНИЙ ВЫВЕРЕН ДИЗАСМОМ и приведён к референсу: реф.
    // KUserOsdSet::GetButtonFunctionId(int) @0x402cb8 — это ПЛОСКИЙ индекс
    // массива, где `index = button*2 + press`, причём **A=0, B=1, M=2** и
    // **Long=0, Short=1** (доказано смещениями в ReadVideoParamConfig @0x4048b8:
    // +0x00 ButtomA/LongPress, +0x04 ButtomA/ShortPress, +0x08 ButtomB/LongPress,
    // +0x0c ButtomB/ShortPress, +0x14 ButtomM/ShortPress).
    // Раньше у нас было {M,A,B} и {Short,Long}. На ЧТЕНИЕ/ЗАПИСЬ это не влияло
    // (значения использовались только как метка switch при сборке имени ключа
    // ini), но ломало бы упакованный индекс — поэтому порядок исправлен ДО того,
    // как появился int-овый вариант ниже.
    enum Button { BTN_A = 0, BTN_B = 1, BTN_M = 2 };
    enum Press  { PRESS_LONG = 0, PRESS_SHORT = 1 };

    void SetConfigFile(const QString &path) { cfgFile_ = path; }
    QString GetUserOsdConfigFile() const;              // osd.ini

    // Список TR-ключей функций по ID (реф. GetFunctionNameList — без фильтра enable).
    static QStringList GetFunctionNameList();
    // TR-ключ функции по ID ("" вне диапазона).
    static QString GetFunctionName(int funcId);
    // Реф. FunctionIdToIndex @0x403e60: ID функции -> ПОЗИЦИЯ в текущем
    // (возможно, отфильтрованном) списке функций.
    // ⚠️ funcId > 11 -> 0; промах по списку или пустой список -> тоже 0.
    // ⚠️ Реф. пропускает ОТРИЦАТЕЛЬНЫЙ funcId (знаковое сравнение) и читает
    // за границей таблицы — у нас отрицательные отсекаются (отступление).
    static int FunctionIdToIndex(int funcId);

    // Кнопка → ID функции (реф. GetButtonFunctionId): [Buttom<X>]/<Short|Long>Press.
    int  GetButtonFunctionId(Button btn, Press press) const;
    // Реф. сигнатура: плоский индекс 0..5 (= button*2 + press).
    // ⚠️ ИНДЕКС 4 (ButtomM/LongPress) в реф. НИКОГДА НЕ ЗАПОЛНЯЕТСЯ читателем
    // osd.ini — он write-only мёртвый конфиг, и KLcdProxy для клавиши 0x217
    // намеренно возвращает 0. Мы читаем его честно (вернётся дефолт).
    // ⚠️ Реф. проверяет границу ЗНАКОВО (`csel ... lt`, bound 7): i >= 7
    // зажимается в 0, а ОТРИЦАТЕЛЬНЫЙ i проходит проверку и читает ЗА
    // границей массива. Мы отрицательные значения отсекаем — воспроизводить
    // выход за границы нельзя (отступление помечено).
    int  GetButtonFunctionId(int packedIndex) const;
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
