#include "db/KDbStrHandler.h"

std::string KDbStrHandler::SqliteReplace(const std::string &str, const std::string &from,
                                         const std::string &to)
{
    if (from.empty())
        return str;
    std::string out = str;
    size_t pos = 0;
    while ((pos = out.find(from, pos)) != std::string::npos) {
        out.replace(pos, from.size(), to);
        pos += to.size();
    }
    return out;
}

std::string KDbStrHandler::SqliteCharsEscape(std::string s)
{
    return SqliteReplace(s, "'", "''");   // реф. — удвоение одинарной кавычки
}

std::string KDbStrHandler::BuildSimpleCondition(const std::string &field,
                                                const std::string &value, const std::string &op)
{
    if (field.empty() || op.empty())
        return std::string();   // реф. guard
    // "field op 'value'" — value сырой, без escape (реф.).
    return field + " " + op + " " + "'" + value + "'";
}

std::string KDbStrHandler::BuildAndCondition(const std::string &a, const std::string &b)
{
    return "(" + a + ") and (" + b + ")";
}

std::string KDbStrHandler::BuildOrCondition(const std::string &a, const std::string &b)
{
    return "(" + a + ") or (" + b + ")";
}
