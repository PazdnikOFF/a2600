#include "kernel/KConfig.h"

#include <QDebug>

#include <cassert>
#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>

#include <unistd.h>

namespace {

// реф. IsValidNumberStr (KConfig.cpp:29) — набор ЛИШЬ "+-.0123456789":
// 'e'/'E' в него НЕ входит, поэтому экспоненциальная запись, которую порождает
// сам FromNumberToString (1.23457e+06), при обратном чтении даёт warning.
// Позиции символов не проверяются: "1.2.3"/"++--"/"" считаются валидными.
bool IsValidNumberStr(const char *str)
{
    assert(nullptr != str);
    return std::strspn(str, "+-.0123456789") == std::strlen(str);
}

// реф. FromStringToBool: UPPERCASE → false только для FALSE/F/NO/N/0, иначе true
// (пустая строка → true, "2"/"off" → true).
bool FromStringToBool(const std::string &v)
{
    std::string u = v;
    for (char &c : u)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return !(u == "FALSE" || u == "F" || u == "NO" || u == "N" || u == "0");
}

// реф. FromNumberToString<T> — ostringstream без манипуляторов
// (дефолт: 6 значащих цифр, defaultfloat). float промотируется в double.
template <typename T>
std::string FromNumberToString(T v)
{
    std::ostringstream os;
    os << v;
    return os.str();
}

// реф. FromStringToNumber<T> — istringstream (не strtod/atoi).
// Реф. при невалидной строке ЛОГИРУЕТ, но раннего выхода НЕТ — парсит всё равно.
template <typename T>
T FromStringToNumber(const std::string &s)
{
    if (!IsValidNumberStr(s.c_str()))
        qDebug() << "Not a valid number str: " << s.c_str();
    T v{};
    std::istringstream is(s);
    is >> v;   // реф. состояние потока не проверяет
    return v;
}

} // namespace

KConfig::KConfig(const std::string &filename) : m_filename(filename)
{
    PreproccessConfigFile();   // реф. ctor
}

KConfig::~KConfig() = default;

// ————— Парсинг —————

bool KConfig::IsFileExisted() const
{
    return ::access(m_filename.c_str(), F_OK) == 0;
}

void KConfig::LoadConfigFile()
{
    std::ifstream f(m_filename, std::ios::binary | std::ios::in);
    if (!f.is_open())
        return;

    f.seekg(0, std::ios::end);
    const std::streamoff n = f.tellg();
    f.seekg(0, std::ios::beg);
    if (n <= 0) {
        m_content.clear();
        return;
    }

    std::vector<char> buf(static_cast<size_t>(n) + 1, '\0');
    f.read(buf.data(), n);
    m_content = buf.data();   // реф. присваивает по strlen → обрыв на первом NUL
}

void KConfig::PreproccessConfigFile()
{
    if (!IsFileExisted())
        return;                 // реф. молчит: пустая карта, без ошибки
    LoadConfigFile();
    EraseComment(m_content);
    m_len = m_content.size();
    ExtractConfigFile();
}

void KConfig::EraseComment(std::string &s) const
{
    // Маркер — только m_comment ('#'). Кавычки не обрабатываются: '#' внутри
    // значения тоже съедается. Стирает от '#' до '\n' ВКЛЮЧАЯ '\n' (реф.) —
    // поэтому строка-комментарий склеивает соседние строки.
    for (;;) {
        const size_t pos = s.find(m_comment);
        if (pos == std::string::npos)
            break;
        const size_t nl = s.find("\n", pos);
        if (nl == std::string::npos)
            s.erase(pos);
        else
            s.erase(pos, nl - pos + 1);
    }
}

void KConfig::TrimBlankSpace(std::string &s)
{
    static const char *blank = " \t\n\v\r\f";
    const size_t b = s.find_first_not_of(blank);
    if (b == std::string::npos) {
        s.clear();
        return;
    }
    const size_t e = s.find_last_not_of(blank);
    s = s.substr(b, e - b + 1);
}

void KConfig::ExtractConfigFile()
{
    m_sections.clear();
    m_cursor = 0;

    for (;;) {
        const size_t open = m_content.find(m_sectionOpen, m_cursor);
        if (open == std::string::npos)
            break;   // всё до первой '[' молча игнорируется (реф.)

        // Блок секции — до следующей '[' или до конца.
        const size_t next  = m_content.find(m_sectionOpen, open + 1);
        const size_t blockEnd = (next == std::string::npos) ? m_content.size() : next;

        const size_t nameEnd = m_content.find(m_sectionClose, open + 1);
        std::string section;
        size_t bodyStart;
        if (nameEnd == std::string::npos || nameEnd >= blockEnd) {
            // '[section' без ']' → имя = весь остаток блока (реф.)
            section   = m_content.substr(open + 1, blockEnd - (open + 1));
            bodyStart = blockEnd;
        } else {
            section   = m_content.substr(open + 1, nameEnd - open - 1);
            bodyStart = nameEnd + 1;
        }
        TrimBlankSpace(section);

        std::map<std::string, std::string> items;
        size_t pos = bodyStart;
        while (pos < blockEnd) {
            const size_t eq = m_content.find(m_delimiter, pos);
            if (eq == std::string::npos || eq >= blockEnd)
                break;   // строка без '=' обрывает разбор блока (реф.)

            // Мусор перед 'key=' поглощается ключом (реф.: trim снимает лишь пробелы).
            std::string key = m_content.substr(pos, eq - pos);
            TrimBlankSpace(key);

            size_t lineEnd = m_content.find("\n", eq + 1);
            if (lineEnd == std::string::npos || lineEnd > blockEnd)
                lineEnd = blockEnd;

            std::string value = m_content.substr(eq + 1, lineEnd - eq - 1);
            TrimBlankSpace(value);

            items[key] = value;   // дубликат ключа → побеждает последний (реф.)
            pos = lineEnd + 1;
        }

        m_sections[section] = items;   // дубликат секции → перезапись целиком (реф.)
        m_cursor = blockEnd;
    }
}

// ————— Чтение —————

bool KConfig::FindValue(const char *section, const char *key, std::string &out) const
{
    if (section == nullptr || key == nullptr)
        return false;
    const auto s = m_sections.find(section);
    if (s == m_sections.end())
        return false;
    const auto k = s->second.find(key);
    if (k == s->second.end())
        return false;      // out НЕ трогаем (реф.)
    out = k->second;
    return true;
}

bool KConfig::HasItem(const char *section, const char *key) const
{
    std::string v;
    return FindValue(section, key, v);
}

void KConfig::ItemMustExist(const char *section, const char *key) const
{
    assert(HasItem(section, key));   // реф. __assert_fail; в NDEBUG — no-op
    (void)section;
    (void)key;
}

std::vector<std::string> KConfig::GetKeysFromSection(const char *section) const
{
    std::vector<std::string> keys;
    if (section == nullptr)
        return keys;                 // реф. nullptr безопасен (→ пустая строка)
    const auto s = m_sections.find(section);
    if (s == m_sections.end())
        return keys;                 // нет секции → пустой вектор
    for (const auto &kv : s->second)
        keys.push_back(kv.first);    // лексикографический порядок (std::map), не файловый
    return keys;
}

bool KConfig::ReadBool(const char *section, const char *key, bool def) const
{
    std::string v;
    return FindValue(section, key, v) ? FromStringToBool(v) : def;
}

int KConfig::ReadInt(const char *section, const char *key, int def) const
{
    std::string v;
    return FindValue(section, key, v) ? FromStringToNumber<int>(v) : def;
}

long long KConfig::ReadLong(const char *section, const char *key, long long def) const
{
    std::string v;
    return FindValue(section, key, v) ? FromStringToNumber<long long>(v) : def;
}

float KConfig::ReadFloat(const char *section, const char *key, float def) const
{
    std::string v;
    return FindValue(section, key, v) ? FromStringToNumber<float>(v) : def;
}

double KConfig::ReadDouble(const char *section, const char *key, double def) const
{
    std::string v;
    return FindValue(section, key, v) ? FromStringToNumber<double>(v) : def;
}

std::string KConfig::ReadString(const char *section, const char *key, const std::string &def) const
{
    std::string v;
    return FindValue(section, key, v) ? v : def;
}

std::string KConfig::ReadData(const char *section, const char *key, const char *def) const
{
    return ReadString(section, key, std::string(def ? def : ""));
}

bool KConfig::ReadDataWithoutDefaultValue(const char *section, const char *key, std::string &out) const
{
    return FindValue(section, key, out);   // реф. — полный синоним FindValue
}

bool KConfig::ReadDataWithoutDefaultValue(const char *section, const char *key, bool &out) const
{
    std::string v;
    if (!FindValue(section, key, v))
        return false;
    out = FromStringToBool(v);
    return true;
}

bool KConfig::ReadDataWithoutDefaultValue(const char *section, const char *key, int &out) const
{
    std::string v;
    if (!FindValue(section, key, v))
        return false;
    out = FromStringToNumber<int>(v);
    return true;
}

bool KConfig::ReadDataWithoutDefaultValue(const char *section, const char *key, long long &out) const
{
    std::string v;
    if (!FindValue(section, key, v))
        return false;
    out = FromStringToNumber<long long>(v);
    return true;
}

bool KConfig::ReadDataWithoutDefaultValue(const char *section, const char *key, float &out) const
{
    std::string v;
    if (!FindValue(section, key, v))
        return false;
    out = FromStringToNumber<float>(v);
    return true;
}

bool KConfig::ReadDataWithoutDefaultValue(const char *section, const char *key, double &out) const
{
    std::string v;
    if (!FindValue(section, key, v))
        return false;
    out = FromStringToNumber<double>(v);
    return true;
}

// ————— Запись —————

void KConfig::WriteData(const char *section, const char *key, const char *value)
{
    if (section == nullptr || key == nullptr)
        return;
    // Секции/ключа нет → создаются; существующее значение — безусловная перезапись.
    m_sections[section][key] = (value ? value : "");
}

void KConfig::WriteData(const char *section, const char *key, bool value)
{
    WriteData(section, key, value ? "True" : "False");   // реф. — именно с заглавной
}

void KConfig::WriteData(const char *section, const char *key, int value)
{
    WriteData(section, key, FromNumberToString(value).c_str());
}

void KConfig::WriteData(const char *section, const char *key, long long value)
{
    WriteData(section, key, FromNumberToString(value).c_str());
}

void KConfig::WriteData(const char *section, const char *key, float value)
{
    // реф. промотирует float в double → форматирование как у double
    WriteData(section, key, FromNumberToString(static_cast<double>(value)).c_str());
}

void KConfig::WriteData(const char *section, const char *key, double value)
{
    WriteData(section, key, FromNumberToString(value).c_str());
}

bool KConfig::Save() const
{
    // реф.: mode _S_bin|_S_out ("wb") — прямая перезапись с усечением,
    // без временного файла и rename (НЕ атомарно).
    std::ofstream f(m_filename, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!f.is_open())
        return false;   // единственная причина false (реф.)

    std::ostringstream os;
    for (const auto &sec : m_sections) {
        os << m_sectionOpen << sec.first << m_sectionClose << "\n";
        for (const auto &kv : sec.second)
            os << kv.first << m_delimiter << kv.second << "\n";
        os << "\n";     // пустая строка после КАЖДОЙ секции, включая последнюю (реф.)
    }
    f << os.str();
    return true;        // ошибка записи/close реф. проглатывается
}
