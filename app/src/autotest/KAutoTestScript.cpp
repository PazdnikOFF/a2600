#include "autotest/KAutoTestScript.h"
#include "sys/KSystem.h"

#include <QDir>
#include <QFile>
#include <QTextStream>

#include <cstring>
#include <cstdlib>
#include <strings.h>   // strncasecmp (реф. использует именно его)

namespace {

// Таблица токенов реф. stKeyValueStr @0x20318 (146 записей × 36 байт {uint32 code; char[32]}).
// Клавиатурная часть = ЗНАЧЕНИЯ Qt::Key (в приёмнике X2000 маскируются &0x0FFFFFFF и идут
// в QtKey2InputKey); панельная часть = малые enum-идентификаторы (та же таблица есть и в
// X2000 @0xa4d448). Опечатки оригинала СОХРАНЕНЫ (см. пометки sic).
struct KeyValueStr { int code; const char *name; };

const KeyValueStr kKeyValueStr[] = {
    // --- клавиатура: Qt::Key ---
    {0x1000000, "Esc"},       {0x1000001, "Tab"},       {0x1000024, "CapsLock"},
    {0x1000004, "Enter"},     {0x1000003, "Backspace"},
    {0x1000030, "F1"},  {0x1000031, "F2"},  {0x1000032, "F3"},  {0x1000033, "F4"},
    {0x1000034, "F5"},  {0x1000035, "F6"},  {0x1000036, "F7"},  {0x1000037, "F8"},
    {0x1000038, "F9"},  {0x1000039, "F10"}, {0x100003a, "F11"}, {0x100003b, "F12"},
    {0x31, "1"}, {0x32, "2"}, {0x33, "3"}, {0x34, "4"}, {0x35, "5"},
    {0x36, "6"}, {0x37, "7"}, {0x38, "8"}, {0x39, "9"}, {0x30, "0"},
    {0x2d, "-"}, {0x3d, "+"},
    {0x51, "Q"}, {0x57, "W"}, {0x45, "E"}, {0x52, "R"}, {0x54, "T"},
    {0x59, "Y"}, {0x55, "U"}, {0x49, "I"}, {0x4f, "O"}, {0x50, "P"},
    {0x41, "A"}, {0x53, "S"}, {0x44, "D"}, {0x46, "F"}, {0x47, "G"},
    {0x48, "H"}, {0x4a, "J"}, {0x4b, "K"}, {0x4c, "L"},
    {0x5a, "Z"}, {0x58, "X"}, {0x43, "C"}, {0x56, "V"}, {0x42, "B"},
    {0x4e, "N"}, {0x4d, "M"},
    {0x2c, "<"}, {0x2e, ">"},
    {0x1000020, "Shift"}, {0x1000021, "Ctrl"}, {0x1000023, "Alt"}, {0x20, "Space"},
    {0x60, "~"}, {0x5b, "{"}, {0x5d, "}"}, {0x3b, ":"}, {0x2c, ","}, {0x5c, "|"}, {0x3f, "?"},
    {0x1000016, "PageUp"},  {0x1000017, "PageDown"},
    {0x1000013, "UP"},      {0x1000015, "DOWN"},
    {0x1000012, "LEFT"},    {0x1000014, "RIGHT"},
    {0x1000022, "Windows"}, {0x1000008, "Pause"},   {0x100000a, "PrtSc"},
    {0x1000006, "Ins"},     {0x1000007, "Delete"},
    {0x1000010, "Home"},    {0x1000011, "End"},
    // --- панель/кастом: малые enum-идентификаторы ---
    {0x00, "GAIN"},   {0x01, "FREEZE"}, {0x02, "CHB"},    {0x03, "RST"},
    {0x04, "ENH"},    {0x05, "COLOR"},  {0x06, "ZOOM"},   {0x07, "IRIS"},
    {0x08, "TONE"},   {0x09, "TONE+"},  {0x0a, "TONE-"},  {0x0b, "SNAP"},
    {0x0c, "USBSTOP"},{0x0d, "WHTBAL"}, {0x0e, "LAMP"},   {0x0f, "LAMP+"},
    {0x10, "AUTO"},   {0x11, "MANU"},   {0x12, "TRANS"},  {0x13, "LEVEL"},
    {0x14, "AIR"},    {0x15, "VLS"},    {0x16, "LAMP-"},  {0x17, "CONTRAST"},
    {0x18, "OPERATING_PATTERN"}, {0x19, "DEMOIRE"},
    {0x20, "POWER"},  {0x21, "VERSION"}, {0x22, "ENDO_INFO"},
    {0x23, "IMAGE_SAVE"}, {0x24, "RECORD"}, {0x25, "VIEW"},
    {0x100, "SWITCH_WINDOW"}, {0x101, "LANGUAGE"},    {0x102, "TIME_FORMAT"},
    {0x103, "VLS_GROUP"},     {0x104, "CONNER_SHAPE"},{0x105, "RESOLUTION"},
    {0x201, "COLOR_R"}, {0x202, "COLOR_B"}, {0x203, "COLOR_C"},
    {0x204, "IMG_ENH_L1"}, {0x205, "IMG_ENH_L2"}, {0x206, "IMG_ENH_L3"},
    {0x207, "COLOR_ENH_L1"}, {0x208, "COLOR_ENH_L2"},
    {0x209, "COLOR_ENH_L2"},   // sic: имя ДУБЛИРУЕТСЯ в прошивке (0x208 и 0x209)
    {0x20a, "ZOOM_L1"}, {0x20b, "ZOOM_L2"}, {0x20c, "ZOOM_L3"},
    {0x20d, "BUTTON0"}, {0x20e, "BUTTON1"}, {0x20f, "BUTTON2"}, {0x210, "BUTTON3"},
    {0x211, "FOOTSWICH_LEFT"}, {0x212, "FOOTSWICH_RIGHT"},   // sic: "SWICH", не "SWITCH"
    {0x213, "BUTTONA_L"}, {0x214, "BUTTONA_S"},
    {0x215, "BUTTONB_L"}, {0x216, "BUTTONB_S"},
    {0x217, "BUTTONM_L"}, {0x218, "BUTTONM_S"},
    // --- псевдо-опкоды ---
    {AUTOTEST_CODE_SLEEP,  "Sleep"},
    {AUTOTEST_CODE_CMD,    "Cmd"},
    {AUTOTEST_CODE_SCRIPT, "Script"},
};

// Префикс без учёта регистра (реф. strncasecmp), сравнение с начала строки.
bool prefixNoCase(const std::string &s, const char *pfx)
{
    const size_t n = std::strlen(pfx);
    if (s.size() < n)
        return false;
    return strncasecmp(s.c_str(), pfx, n) == 0;
}

int toInt(const std::string &s) { return std::atoi(s.c_str()); }   // реф. atoi

} // namespace

QString KAutoTestScript::AutotestDir()
{
    // Реф. литерал "/home/root/data/app/autotest/".
    return QDir(KSystem::AppPath()).absoluteFilePath("autotest") + "/";
}

void KAutoTestScript::ResetKeyValue(KEY_CONFIG &key)
{
    key = KEY_CONFIG();   // code=-1, целые 0, cnt=1, cmd пуст (реф. ResetKeyValue)
}

std::string KAutoTestScript::GetCaseValue(const std::string &line, bool *found)
{
    const size_t eq = line.find('=');
    if (eq == std::string::npos) {
        if (found) *found = false;   // реф. вернул бы NULL (и упал бы у вызывающего)
        return std::string();
    }
    size_t i = eq + 1;
    while (i < line.size() && line[i] == ' ')   // реф. пропускает ТОЛЬКО пробелы
        ++i;
    std::string v = line.substr(i);
    // Реф. режет последний символ (strlen-1), снимая '\n'. Здесь строки уже без '\n',
    // поэтому режем только фактические хвостовые CR/LF.
    while (!v.empty() && (v.back() == '\n' || v.back() == '\r'))
        v.pop_back();
    if (found) *found = true;
    return v;
}

int KAutoTestScript::KeyValueFromName(const std::string &name)
{
    for (const KeyValueStr &e : kKeyValueStr)
        if (name == e.name)      // точное сравнение (регистр учитывается)
            return e.code;
    return -1;                   // не распознан → OneKeyExec вернул бы false
}

int KAutoTestScript::KeyValueTableSize()
{
    return static_cast<int>(sizeof(kKeyValueStr) / sizeof(kKeyValueStr[0]));
}

int KAutoTestScript::AnalyseLineCase(const std::string &line, KEY_CONFIG &key, INI_CONFIG &ini)
{
    // Порядок проверок — как в реф.; префиксы без учёта регистра.
    if (prefixNoCase(line, "[Key"))        // ЛЮБОЙ "[Key…", включая "[KeyEnd]"; номер не парсится
        return LINE_KEY;
    if (prefixNoCase(line, "[Confiure]"))  // опечатка оригинала
        return LINE_CONFIURE;
    if (prefixNoCase(line, "env"))         // значение ОТБРАСЫВАЕТСЯ (реф.)
        return LINE_ENV;
    if (prefixNoCase(line, "num")) { ini.num  = toInt(GetCaseValue(line)); return LINE_NUM; }
    if (prefixNoCase(line, "time")){ ini.time = toInt(GetCaseValue(line)); return LINE_TIME; }

    if (prefixNoCase(line, "code")) {
        key.code = KeyValueFromName(GetCaseValue(line));
        return LINE_FIELD;
    }
    if (prefixNoCase(line, "event")) { key.event = toInt(GetCaseValue(line)); return LINE_FIELD; }
    if (prefixNoCase(line, "ctrl"))  { key.ctrl  = toInt(GetCaseValue(line)); return LINE_FIELD; }
    if (prefixNoCase(line, "alt"))   { key.alt   = toInt(GetCaseValue(line)); return LINE_FIELD; }
    if (prefixNoCase(line, "shift")) { key.shift = toInt(GetCaseValue(line)); return LINE_FIELD; }
    if (prefixNoCase(line, "value")) { key.value = toInt(GetCaseValue(line)); return LINE_FIELD; }
    if (prefixNoCase(line, "sleep")) { key.sleep = toInt(GetCaseValue(line)); return LINE_FIELD; }
    if (prefixNoCase(line, "cnt"))   { key.cnt   = toInt(GetCaseValue(line)); return LINE_FIELD; }
    if (prefixNoCase(line, "cmd"))   { key.cmd   = GetCaseValue(line);        return LINE_FIELD; }
    return LINE_UNKNOWN;                    // неизвестные строки молча игнорируются
}

bool KAutoTestScript::INIFileCaseExec(const QString &file, QVector<KEY_CONFIG> &outKeys,
                                      INI_CONFIG &outIni)
{
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    QTextStream ts(&f);

    KEY_CONFIG key;
    ResetKeyValue(key);
    bool anyKey = false;   // первый "[Key" только открывает блок, сбрасывать нечего

    while (!ts.atEnd()) {
        std::string line = ts.readLine().toStdString();
        // Реф. срезает ТОЛЬКО ведущие пробелы (не табы).
        size_t b = 0;
        while (b < line.size() && line[b] == ' ')
            ++b;
        line = line.substr(b);

        if (AnalyseLineCase(line, key, outIni) != LINE_KEY)
            continue;
        // "[Key…" СБРАСЫВАЕТ предыдущий накопленный шаг (реф.: flush-on-next-header).
        if (anyKey) {
            if (key.code == AUTOTEST_CODE_SCRIPT && !key.cmd.empty()) {
                // Рекурсия: под-скрипт AutotestDir()+cmd, повторяется cnt раз.
                // Ни ограничения глубины, ни защиты от циклов в реф. НЕТ — сохраняем 1:1.
                const QString sub = AutotestDir() + QString::fromStdString(key.cmd);
                for (int i = 0; i < key.cnt; ++i) {
                    INI_CONFIG subIni;
                    INIFileCaseExec(sub, outKeys, subIni);
                }
            } else {
                outKeys.append(key);   // реф. здесь вызвал бы OneKeyExec → SendOneKey (device)
            }
        }
        anyKey = true;
        ResetKeyValue(key);
    }
    // КВИРК РЕФ. (сохранён намеренно): ПОСЛЕ цикла накопленный шаг НЕ сбрасывается.
    // Последний блок исполняется ТОЛЬКО если в файле есть завершающий заголовок "[Key…"
    // — именно поэтому скрипты прошивки заканчиваются на [KeyEnd]. Без него последний
    // шаг молча теряется. Дополнительного flush здесь быть НЕ ДОЛЖНО.
    f.close();
    return true;
}
