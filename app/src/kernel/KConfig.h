#pragma once

#include <map>
#include <string>
#include <vector>

// INI-движок ядра (реф. architecture/src/kernel/ini/KConfig.cpp, X-2600).
// База для *ConfigSetupHandler-фасадов (patientsetup.ini/worklistsetup.ini и др.).
//
// ВАЖНО — семантика отличается от QSettings, поэтому реализован отдельно:
//   * bool сериализуется как "True"/"False" (не "true"/"false", не 1/0);
//   * при чтении bool регистронезависим: false ТОЛЬКО для FALSE/F/NO/N/0,
//     всё прочее (включая "" и "2") → true;
//   * комментарий — только '#' (';' и '//' НЕ поддерживаются), вырезается
//     вместе с завершающим \n;
//   * Save() перезаписывает файл целиком, секции/ключи — в лексикографическом
//     порядке (std::map), пустая строка после КАЖДОЙ секции; комментарии и
//     текст до первой секции НЕ сохраняются;
//   * WriteData правит только карту в памяти — на диск пишет лишь Save().
//
// Имена полей в оригинале невосстановимы (нет DWARF для кода приложения) —
// смещения/типы взяты из дизасма, имена наши.
class KConfig
{
public:
    explicit KConfig(const std::string &filename);   // реф. ctor зовёт PreproccessConfigFile()
    virtual ~KConfig();                              // единственный virtual (vtable 4 слота)

    // --- Парсинг ---
    void LoadConfigFile();                           // файл → m_content (обрыв на первом NUL)
    void PreproccessConfigFile();                    // Load + EraseComment + Extract
    void ExtractConfigFile();                        // m_content → m_sections
    void TrimBlankSpace(std::string &s);             // " \t\n\v\r\f" с обоих концов
    void EraseComment(std::string &s) const;         // от '#' до '\n' включительно
    bool IsFileExisted() const;                      // access(F_OK)

    // --- Чтение ---
    bool FindValue(const char *section, const char *key, std::string &out) const;  // ядро
    bool HasItem(const char *section, const char *key) const;
    void ItemMustExist(const char *section, const char *key) const;  // реф. assert (в NDEBUG — no-op)
    std::vector<std::string> GetKeysFromSection(const char *section) const;        // лексикогр.

    bool        ReadBool  (const char *section, const char *key, bool def) const;
    int         ReadInt   (const char *section, const char *key, int def) const;
    long long   ReadLong  (const char *section, const char *key, long long def) const;
    float       ReadFloat (const char *section, const char *key, float def) const;
    double      ReadDouble(const char *section, const char *key, double def) const;
    std::string ReadString(const char *section, const char *key, const std::string &def) const;

    // ReadData — алиасы (в реф. тонкие tail-jump'ы на соответствующие Read*).
    std::string ReadData(const char *section, const char *key, const char *def) const;
    bool        ReadData(const char *section, const char *key, bool def) const      { return ReadBool(section, key, def); }
    int         ReadData(const char *section, const char *key, int def) const       { return ReadInt(section, key, def); }
    long long   ReadData(const char *section, const char *key, long long def) const { return ReadLong(section, key, def); }
    float       ReadData(const char *section, const char *key, float def) const     { return ReadFloat(section, key, def); }
    double      ReadData(const char *section, const char *key, double def) const    { return ReadDouble(section, key, def); }

    // При отсутствии ключа out НЕ модифицируется (реф.).
    bool ReadDataWithoutDefaultValue(const char *section, const char *key, std::string &out) const;
    bool ReadDataWithoutDefaultValue(const char *section, const char *key, bool &out) const;
    bool ReadDataWithoutDefaultValue(const char *section, const char *key, int &out) const;
    bool ReadDataWithoutDefaultValue(const char *section, const char *key, long long &out) const;
    bool ReadDataWithoutDefaultValue(const char *section, const char *key, float &out) const;
    bool ReadDataWithoutDefaultValue(const char *section, const char *key, double &out) const;

    // --- Запись (только в память; на диск — Save) ---
    void WriteData(const char *section, const char *key, const char *value);   // ядро
    void WriteData(const char *section, const char *key, bool value);          // → "True"/"False"
    void WriteData(const char *section, const char *key, int value);
    void WriteData(const char *section, const char *key, long long value);
    void WriteData(const char *section, const char *key, float value);         // ostringstream, 6 знач. цифр
    void WriteData(const char *section, const char *key, double value);

    bool Save() const;   // false ТОЛЬКО если файл не открылся (реф.)

private:
    // Разделители — в реф. поля объекта (Save/Extract читают их из this), не литералы.
    std::string m_comment      = "#";    // +0x08
    std::string m_delimiter    = "=";    // +0x28
    std::string m_sectionOpen  = "[";    // +0x48
    std::string m_sectionClose = "]";    // +0x68
    std::string m_filename;              // +0x88
    size_t      m_len = 0;               // +0xa8 (длина контента после EraseComment)
    std::string m_content;               // +0xb0 (весь текст файла)
    std::map<std::string, std::map<std::string, std::string>> m_sections;  // +0xd0
    size_t      m_cursor = 0;            // +0x100 (курсор парсинга)
};
