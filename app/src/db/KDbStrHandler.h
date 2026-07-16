#pragma once

#include <string>

// Построитель SQL-условий (реф. KDbStrHandler, X-2600). Чистые статические строковые
// функции, не UI/не device. Используется генерик-слоем БД оригинала для сборки WHERE.
//
// ВНИМАНИЕ: BuildSimpleCondition вставляет value СЫРЫМ, без экранирования (реф. не
// зовёт SqliteCharsEscape внутри — экранирование на совести вызывающего). Наш
// connection-based слой использует плейсхолдеры QSqlQuery (безопаснее); этот класс —
// для мест, где нужен точный SQL-текст условия как в оригинале.
class KDbStrHandler
{
public:
    // Replace-all from→to.
    static std::string SqliteReplace(const std::string &str, const std::string &from,
                                     const std::string &to);
    // Экранирование для SQLite: удвоение одинарной кавычки (' → '').
    static std::string SqliteCharsEscape(std::string s);

    // "field op 'value'". Пустой field ИЛИ op → "". value НЕ экранируется (реф.).
    // Порядок аргументов реф.: (field, value, op).
    static std::string BuildSimpleCondition(const std::string &field, const std::string &value,
                                            const std::string &op);
    // "(a) and (b)"
    static std::string BuildAndCondition(const std::string &a, const std::string &b);
    // "(a) or (b)"
    static std::string BuildOrCondition(const std::string &a, const std::string &b);
};
