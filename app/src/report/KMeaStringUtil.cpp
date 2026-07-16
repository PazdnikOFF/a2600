#include "report/KMeaStringUtil.h"

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <regex>
#include <sstream>

namespace {
// реф. ConvertFromInt/Double/String — все через stringstream, без проверок.
template <typename T>
std::string toStr(T v)
{
    std::stringstream ss;
    std::string out;
    ss << v;
    ss >> out;
    return out;
}

template <typename T>
T fromStr(const std::string &s)
{
    std::stringstream ss;
    T v{};
    ss << s;
    ss >> v;   // реф. fail() НЕ проверяет: "abc" → 0, "12abc" → 12
    return v;
}
} // namespace

void KMeaStringUtil::StringToUpper(std::string &str)
{
    for (char &c : str)
        c = static_cast<char>(::toupper(static_cast<unsigned char>(c)));
}

void KMeaStringUtil::StringToLower(std::string &str)
{
    for (char &c : str)
        c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
}

std::string &KMeaStringUtil::ReplaceStr(std::string &str, const std::string &from,
                                        const std::string &to)
{
    if (from.empty())
        return str;   // реф. — мгновенный выход (иначе вечный цикл)
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos) {
        str.replace(pos, from.size(), to);
        pos += to.size();   // реф. не перепросматривает вставленное
    }
    return str;
}

void KMeaStringUtil::DeleteChars(char *str, const char *chars)
{
    if (str == nullptr || chars == nullptr)
        return;
    int tbl[256] = {0};
    for (const char *p = chars; *p; ++p)
        tbl[static_cast<unsigned char>(*p)] = 1;

    char *dst = str;
    for (const char *src = str; *src; ++src) {
        if (!tbl[static_cast<unsigned char>(*src)])
            *dst++ = *src;
    }
    *dst = '\0';
}

bool KMeaStringUtil::IsChineseChar(const std::string &str, int idx)
{
    const int n = static_cast<int>(str.size());
    if (n == 0 || idx < 0 || idx >= n)
        return false;
    auto hi = [](char c) { return (c & 0x80) != 0; };
    if (!hi(str[idx]))
        return false;
    if (idx == 0)
        return hi(str.c_str()[1]);   // при size==1 читает завершающий '\0' → false
    if (idx == n - 1)
        return hi(str[idx - 1]);
    return hi(str[idx + 1]) || hi(str[idx - 1]);
}

std::string KMeaStringUtil::ConvertIntToString(int value)
{
    return toStr(value);
}

std::string KMeaStringUtil::ConvertDoubleToString(double value)
{
    return toStr(value);   // 6 знач. цифр (НЕ std::to_string): 100.0 → "100"
}

double KMeaStringUtil::ConvertStringToDouble(const std::string &str)
{
    return fromStr<double>(str);
}

int KMeaStringUtil::ConvertStringToInt(const std::string &str)
{
    return fromStr<int>(str);
}

bool KMeaStringUtil::IsBeginWith(const std::string &str, const std::string &prefix)
{
    if (str.size() < prefix.size())
        return false;
    return str.substr(0, prefix.size()) == prefix;   // пустой prefix → true
}

bool KMeaStringUtil::IsEndWith(const std::string &str, const std::string &suffix)
{
    if (str.size() < suffix.size())
        return false;
    return str.substr(str.size() - suffix.size()) == suffix;   // пустой suffix → true
}

std::string KMeaStringUtil::FormatStr(const char *fmt, ...)
{
    char buf[1024] = {0};
    va_list ap;
    va_start(ap, fmt);
    ::vsnprintf(buf, sizeof(buf), fmt, ap);   // реф.: обрезка до 1023 символов
    va_end(ap);
    return std::string(buf, ::strlen(buf));   // длина по strlen, не по возврату vsnprintf
}

std::vector<std::string> KMeaStringUtil::SplitStr(const std::string &str,
                                                  const std::string &delims)
{
    std::vector<std::string> out;
    if (delims.empty())
        return out;   // реф.: пустой набор → ПУСТОЙ вектор, а не {str}

    size_t start = 0;
    for (;;) {
        const size_t pos = str.find_first_of(delims, start);   // НАБОР символов
        if (pos == std::string::npos)
            break;
        if (pos > start)
            out.push_back(str.substr(start, pos - start));     // пустые токены пропускаются
        start = pos + 1;
    }
    if (start < str.size())
        out.push_back(str.substr(start));
    return out;
}

std::vector<std::string> KMeaStringUtil::SplitStr2(const std::string &str,
                                                   const std::string &delim)
{
    std::vector<std::string> out;
    if (delim.empty())
        return out;

    size_t start = 0;
    for (;;) {
        const size_t pos = str.find(delim, start);   // ПОДСТРОКА
        if (pos == std::string::npos)
            break;
        if (pos > start)
            out.push_back(str.substr(start, pos - start));
        start = pos + delim.size();
    }
    if (start < str.size())
        out.push_back(str.substr(start));
    return out;
}

std::string KMeaStringUtil::ConvertIntToFormatString(int value, int width)
{
    const std::string fmt = "%0" + toStr(width) + "d";
    // Реф. держит буфер 100 и делает buf[snprintf(...)] = 0 → при width>=100 порча
    // стека. Баг НЕ воспроизводим: считаем нужный размер и пишем корректно.
    const int n = ::snprintf(nullptr, 0, fmt.c_str(), value);
    if (n < 0)
        return std::string();
    std::vector<char> buf(static_cast<size_t>(n) + 1, '\0');
    ::snprintf(buf.data(), buf.size(), fmt.c_str(), value);
    return std::string(buf.data(), ::strlen(buf.data()));
}

void KMeaStringUtil::ReplaceIllegalChar(std::string &str, const std::string &repl)
{
    // Порядок реф.: \ / : * ? " > < |
    static const char *illegal[] = {"\\", "/", ":", "*", "?", "\"", ">", "<", "|"};
    for (const char *c : illegal) {
        const size_t pos = str.find(c, 0);   // реф. — ОДИН find на символ
        if (pos != std::string::npos)
            str.replace(pos, 1, repl);       // → заменяется только первое вхождение
    }
}

std::string KMeaStringUtil::TrimBeginEndStrRef(std::string &str)
{
    if (str.empty())
        return std::string();
    // Реф. тримит ТОЛЬКО пробел ' ' — \t\n\v\r\f не трогает.
    const size_t b = str.find_first_not_of(' ');
    if (b == std::string::npos) {
        str.clear();
        return std::string();
    }
    str.erase(0, b);
    const size_t e = str.find_last_not_of(' ');
    if (e != std::string::npos)
        str.erase(e + 1);
    return str;
}

std::string KMeaStringUtil::TrimBeginEndStr(const std::string &str)
{
    std::string copy = str;
    return TrimBeginEndStrRef(copy);
}

void KMeaStringUtil::TrimAllStr(std::string &str)
{
    // реф. boost::erase_all ×3: удаляет ОТОВСЮДУ, и только эти три символа.
    for (char c : {' ', '\t', '\n'})
        str.erase(std::remove(str.begin(), str.end(), c), str.end());
}

bool KMeaStringUtil::IsEqual(const std::string &a, const std::string &b, bool bCaseSensitive)
{
    if (bCaseSensitive)
        return a == b;
    std::string la = a, lb = b;
    StringToLower(la);
    StringToLower(lb);
    return la == lb;
}

void KMeaStringUtil::ReadChars(char *str, std::vector<std::string> &keys,
                               std::vector<std::string> &values)
{
    if (str == nullptr || *str == '\0')
        return;
    // Реф.: векторы НЕ очищаются (append). Пустые токены сохраняются.
    // Хвост без завершающей ';' теряется — токен просто уничтожается.
    std::string tok;
    for (const char *p = str; *p; ++p) {
        if (*p == ':') {
            keys.push_back(tok);
            tok.clear();
        } else if (*p == ';') {
            values.push_back(tok);
            tok.clear();
        } else {
            tok.push_back(*p);
        }
    }
}

std::string KMeaStringUtil::SearchStr(const std::string &str, const std::string &left,
                                      const std::string &right)
{
    // Реф. НЕ экранирует left/right — они трактуются как regex (метасимволы
    // меняют смысл, невалидный шаблон бросает std::regex_error наружу).
    const std::regex re(".*?" + left + "(.*?)" + right + ".*?");
    std::smatch m;
    if (!std::regex_search(str, m, re) || !m[1].matched)
        return std::string();
    return m[1].str();
}
