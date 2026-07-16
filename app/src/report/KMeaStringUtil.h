#pragma once

#include <string>
#include <vector>

// Строковые утилиты (реф. KMeaStringUtil, X-2600). Потребители — отчётная ветка:
// report_template::ConvertStringToMap/AppendCustomedItem, KTextBlock::FontSize/Title,
// KImageBlock::Url/Width/Heigth, KTableBlock::Margin, KRTDataSourceReal::GetTextData…
//
// Класс ПУСТОЙ и БЕЗ СОСТОЯНИЯ, но методы НЕ static (реф. ctor/dtor = голый ret,
// vtable нет). Потребители делают `KMeaStringUtil util; util.SplitStr(...)` —
// сохраняем это 1:1.
//
// ВНИМАНИЕ, семантика расходится с «интуитивной» (сверено дизасмом):
//   * SplitStr — разделитель это НАБОР СИМВОЛОВ (find_first_of), а SplitStr2 —
//     ПОДСТРОКА (find); оба пропускают пустые токены и НЕ тримят;
//   * пустой разделитель → ПУСТОЙ вектор (а не весь вход одним токеном);
//   * TrimBeginEndStr* тримит ТОЛЬКО пробел ' ' — не \t\n\v\r\f;
//   * TrimAllStr удаляет ' ', '\t', '\n' ОТОВСЮДУ, а не с концов;
//   * ConvertStringTo{Int,Double} без валидации: "abc"→0, "12abc"→12, "0x10"→0;
//   * ConvertDoubleToString — stringstream (6 знач. цифр), НЕ std::to_string;
//   * IsBeginWith/IsEndWith с пустым префиксом/суффиксом → true;
//   * ReplaceIllegalChar меняет только ПЕРВОЕ вхождение каждого символа.
class KMeaStringUtil
{
public:
    KMeaStringUtil() = default;
    ~KMeaStringUtil() = default;

    void StringToUpper(std::string &str);   // побайтно toupper (UTF-8 ломается — как в реф.)
    void StringToLower(std::string &str);   // побайтно tolower

    // Заменяет ВСЕ вхождения. Пустой from → выход без изменений (защита от вечного цикла).
    std::string &ReplaceStr(std::string &str, const std::string &from, const std::string &to);

    // In-place удаление всех символов из набора chars. nullptr-аргументы → no-op.
    void DeleteChars(char *str, const char *chars);

    // «Байт idx — часть многобайтного символа»: hi(s[idx]) && (hi(s[idx±1])).
    bool IsChineseChar(const std::string &str, int idx);

    std::string ConvertIntToString(int value);
    std::string ConvertDoubleToString(double value);   // 6 знач. цифр: 1234567.0 → "1.23457e+06"
    double ConvertStringToDouble(const std::string &str);   // без валидации → 0 при мусоре
    int    ConvertStringToInt(const std::string &str);      // без валидации → 0 при мусоре

    bool IsBeginWith(const std::string &str, const std::string &prefix);
    bool IsEndWith(const std::string &str, const std::string &suffix);

    std::string FormatStr(const char *fmt, ...);   // буфер 1024, обрезка до 1023

    // Разделитель — НАБОР СИМВОЛОВ (find_first_of). "a,,b" + "," → {"a","b"}.
    std::vector<std::string> SplitStr(const std::string &str, const std::string &delims);
    // Разделитель — ПОДСТРОКА (find).
    std::vector<std::string> SplitStr2(const std::string &str, const std::string &delim);

    // "%0<width>d". ВНИМАНИЕ: реф. при width>=100 пишет за 100-байтовый буфер
    // (buf[snprintf()] — порча стека). Баг не воспроизводим: у нас буфер по размеру.
    std::string ConvertIntToFormatString(int value, int width);

    // Заменяет ПЕРВОЕ вхождение каждого из 9 символов: \ / : * ? " > < |
    void ReplaceIllegalChar(std::string &str, const std::string &repl);

    std::string TrimBeginEndStrRef(std::string &str);        // тримит in-place И возвращает копию
    std::string TrimBeginEndStr(const std::string &str);     // на копии, вход не меняется
    void TrimAllStr(std::string &str);                       // удаляет ' ', '\t', '\n' отовсюду

    bool IsEqual(const std::string &a, const std::string &b, bool bCaseSensitive);

    // Парсер "key:value;key:value;". Векторы НЕ очищаются (append), пустые токены
    // СОХРАНЯЮТСЯ, а без завершающей ';' последний value ТЕРЯЕТСЯ (как в реф.).
    void ReadChars(char *str, std::vector<std::string> &keys, std::vector<std::string> &values);

    // regex ".*?" + left + "(.*?)" + right + ".*?" → возврат группы 1.
    // left/right НЕ экранируются (реф.): метасимволы меняют смысл, битый regex бросает.
    std::string SearchStr(const std::string &str, const std::string &left,
                          const std::string &right);
};
