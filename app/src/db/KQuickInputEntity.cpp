#include "db/KQuickInputEntity.h"

namespace {
// Реф. кладёт int'ы в map как десятичный текст (generic-слой хранит только строки).
std::string I2S(int v)
{
    return std::to_string(v);
}
}   // namespace

std::map<std::string, std::string> KQIPEntity::ConvertToMap() const
{
    // Реф. порядок появления ключей: mKey, id, name, sex, birthday, age, count, time.
    // mKey == -1 (не назначен) в map НЕ кладётся — иначе INSERT затрёт автоключ.
    std::map<std::string, std::string> m;
    if (mKey >= 0)
        m["mKey"] = I2S(mKey);
    m["id"]       = id;
    m["name"]     = name;
    m["sex"]      = I2S(sex);
    m["birthday"] = birthday;
    m["age"]      = I2S(age);
    m["count"]    = I2S(count);
    m["time"]     = time;
    return m;
}

std::map<std::string, std::string> KQIDEntity::ConvertToMap() const
{
    std::map<std::string, std::string> m;
    if (mKey >= 0)
        m["mKey"] = I2S(mKey);
    m["name"]    = name;
    m["account"] = account;
    m["count"]   = I2S(count);
    m["time"]    = time;
    return m;
}

std::map<std::string, std::string> KQIAEntity::ConvertToMap() const
{
    std::map<std::string, std::string> m;
    if (mKey >= 0)
        m["mKey"] = I2S(mKey);
    m["name"]  = name;
    m["count"] = I2S(count);
    m["time"]  = time;
    return m;
}
