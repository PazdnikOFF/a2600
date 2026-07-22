#include "db/KQuickInputStore.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace {

std::string Trim(const std::string &s)
{
    size_t a = 0, b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return s.substr(a, b - a);
}

std::string Lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// Снять ровно одну внешнюю пару скобок, если она обрамляет ВСЁ выражение.
std::string Unwrap(const std::string &in)
{
    std::string s = Trim(in);
    while (s.size() >= 2 && s.front() == '(' && s.back() == ')') {
        int depth = 0;
        bool wraps = true;
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '(') ++depth;
            else if (s[i] == ')') {
                --depth;
                if (depth == 0 && i + 1 != s.size()) { wraps = false; break; }
            }
        }
        if (!wraps)
            break;
        s = Trim(s.substr(1, s.size() - 2));
    }
    return s;
}

// Найти верхнеуровневый (вне скобок и вне кавычек) оператор ` and ` / ` or `.
// Возвращает позицию или npos. Ищем справа налево — левая ассоциативность не важна.
size_t FindTopLevel(const std::string &s, const std::string &op)
{
    int depth = 0;
    bool inQuote = false;
    for (size_t i = 0; i < s.size(); ++i) {
        const char c = s[i];
        if (c == '\'') { inQuote = !inQuote; continue; }
        if (inQuote) continue;
        if (c == '(') { ++depth; continue; }
        if (c == ')') { --depth; continue; }
        if (depth == 0 && i + op.size() <= s.size() && Lower(s.substr(i, op.size())) == op)
            return i;
    }
    return std::string::npos;
}

// SQL-LIKE: '%' — любая последовательность, '_' — любой одиночный символ. Сравнение
// регистрозависимое (как у SQLite для не-ASCII; для наших тестов достаточно).
bool LikeMatch(const std::string &value, const std::string &pattern)
{
    // Классический итеративный алгоритм с откатом по последней '%'.
    size_t v = 0, p = 0, star = std::string::npos, mark = 0;
    while (v < value.size()) {
        if (p < pattern.size() && (pattern[p] == '_' || pattern[p] == value[v])) {
            ++v; ++p;
        } else if (p < pattern.size() && pattern[p] == '%') {
            star = p++;
            mark = v;
        } else if (star != std::string::npos) {
            p = star + 1;
            v = ++mark;
        } else {
            return false;
        }
    }
    while (p < pattern.size() && pattern[p] == '%')
        ++p;
    return p == pattern.size();
}

// Оценить условие вида "field = 'value'" / "field LIKE 'pat'" / "(A) and (B)" / "(A) or (B)".
// Всё, что не разобрано, — false (стаб честно не притворяется SQL-движком).
bool Eval(const std::string &cond, const std::map<std::string, std::string> &row)
{
    const std::string s = Unwrap(cond);
    if (s.empty())
        return true;   // пустое условие — без фильтра

    size_t p = FindTopLevel(s, " or ");
    if (p != std::string::npos)
        return Eval(s.substr(0, p), row) || Eval(s.substr(p + 4), row);
    p = FindTopLevel(s, " and ");
    if (p != std::string::npos)
        return Eval(s.substr(0, p), row) && Eval(s.substr(p + 5), row);

    // «field LIKE 'pattern'» (используется GetMatchDate).
    const size_t like = FindTopLevel(s, " like ");
    if (like != std::string::npos) {
        const std::string field = Trim(s.substr(0, like));
        std::string pat = Trim(s.substr(like + 6));
        if (pat.size() >= 2 && pat.front() == '\'' && pat.back() == '\'')
            pat = pat.substr(1, pat.size() - 2);
        auto it = row.find(field);
        return it != row.end() && LikeMatch(it->second, pat);
    }

    const size_t eq = s.find('=');
    if (eq == std::string::npos)
        return false;
    const std::string field = Trim(s.substr(0, eq));
    std::string value = Trim(s.substr(eq + 1));
    if (value.size() >= 2 && value.front() == '\'' && value.back() == '\'')
        value = value.substr(1, value.size() - 2);
    auto it = row.find(field);
    return it != row.end() && it->second == value;
}

std::string Get(const std::map<std::string, std::string> &m, const std::string &k)
{
    auto it = m.find(k);
    return it == m.end() ? std::string() : it->second;
}

}   // namespace

int KQuickInputMemStore::CreateEntity(const std::string &table,
                                      const std::map<std::string, std::string> &fields)
{
    if (table.empty())
        return -1;
    Row r = fields;
    int key = m_nextKey++;
    auto it = fields.find("mKey");
    if (it != fields.end() && !it->second.empty())
        key = std::atoi(it->second.c_str());
    r["mKey"] = std::to_string(key);
    m_tables[table].push_back(r);
    return key;
}

bool KQuickInputMemStore::UpdateEntity(const std::string &table, const std::string &key,
                                       const std::map<std::string, std::string> &fields)
{
    auto t = m_tables.find(table);
    if (t == m_tables.end())
        return false;
    bool any = false;
    for (Row &r : t->second) {
        if (Get(r, "mKey") != key)
            continue;
        for (const auto &kv : fields)
            if (kv.first != "mKey")
                r[kv.first] = kv.second;
        any = true;
    }
    return any;
}

bool KQuickInputMemStore::DeleteEntity(const std::string &table, const std::string &key)
{
    auto t = m_tables.find(table);
    if (t == m_tables.end())
        return false;
    const size_t before = t->second.size();
    t->second.erase(std::remove_if(t->second.begin(), t->second.end(),
                                   [&](const Row &r) { return Get(r, "mKey") == key; }),
                    t->second.end());
    return t->second.size() != before;
}

bool KQuickInputMemStore::GetEntityDetailList(const std::string &table,
                                              const std::map<std::string, std::string> &spec,
                                              std::vector<Row> &out)
{
    auto t = m_tables.find(table);
    if (t == m_tables.end())
        return false;

    const std::string where = Get(spec, "Where");
    const std::string order = Get(spec, "Order");
    const std::string limit = Get(spec, "Limit");

    Rows rows;
    for (const Row &r : t->second) {
        bool ok = true;
        // Обычные ключи спеки — равенство по колонке (спец-ключи пропускаем).
        for (const auto &kv : spec) {
            if (kv.first == "Where" || kv.first == "Order" || kv.first == "Limit"
                || kv.first == "Column" || kv.first == "Group")
                continue;
            if (Get(r, kv.first) != kv.second) { ok = false; break; }
        }
        if (ok && !where.empty())
            ok = Eval(where, r);
        if (ok)
            rows.push_back(r);
    }

    // ORDER BY «col DIR, col DIR» — стабильная сортировка от последнего ключа к первому.
    if (!order.empty()) {
        std::vector<std::pair<std::string, bool>> keys;   // (колонка, по убыванию?)
        size_t pos = 0;
        while (pos <= order.size()) {
            const size_t comma = order.find(',', pos);
            const std::string part = Trim(order.substr(pos, comma == std::string::npos
                                                                ? std::string::npos
                                                                : comma - pos));
            if (!part.empty()) {
                const size_t sp = part.find(' ');
                const std::string col = Trim(part.substr(0, sp));
                const bool desc = sp != std::string::npos
                                  && Lower(Trim(part.substr(sp + 1))) == "desc";
                keys.emplace_back(col, desc);
            }
            if (comma == std::string::npos)
                break;
            pos = comma + 1;
        }
        for (auto it = keys.rbegin(); it != keys.rend(); ++it) {
            const std::string col = it->first;
            const bool desc = it->second;
            std::stable_sort(rows.begin(), rows.end(), [&](const Row &a, const Row &b) {
                const std::string va = Get(a, col), vb = Get(b, col);
                return desc ? vb < va : va < vb;
            });
        }
    }

    if (!limit.empty()) {
        const int n = std::atoi(limit.c_str());
        if (n >= 0 && rows.size() > static_cast<size_t>(n))
            rows.resize(static_cast<size_t>(n));
    }

    out.insert(out.end(), rows.begin(), rows.end());   // реф.: out НЕ очищается
    return true;
}

int KQuickInputMemStore::GetRecordsNumber(const std::string &table, const std::string &where)
{
    auto t = m_tables.find(table);
    if (t == m_tables.end())
        return 0;
    int n = 0;
    for (const Row &r : t->second)
        if (where.empty() || Eval(where, r))
            ++n;
    return n;
}
