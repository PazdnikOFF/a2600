#include "db/KDbSqlite.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

bool KDbSqlite::m_bIsLogOn = false;

namespace {
const int SQL_BUF_SIZE = 0xa000;   // реф. — фикс. буфер snprintf 40960 байт
}

KDbSqlite::KDbSqlite()
{
    // реф. ctor: sqlite3_mutex_alloc(SQLITE_MUTEX_RECURSIVE) (arg=1).
    m_pMutex = sqlite3_mutex_alloc(SQLITE_MUTEX_RECURSIVE);
}

KDbSqlite::~KDbSqlite()
{
    // реф. dtor: Close() → sqlite3_mutex_free → destroy строк.
    Close();
    if (m_pMutex) {
        sqlite3_mutex_free(m_pMutex);
        m_pMutex = nullptr;
    }
}

int KDbSqlite::Open(const std::string &path)
{
    m_strDbPath = path;
    sqlite3_mutex_enter(m_pMutex);
    int rc = sqlite3_open(path.c_str(), &m_pDb);
    while (rc == SQLITE_BUSY) {            // реф. ретрай BUSY со sleep(100)
        sqlite3_sleep(100);
        rc = sqlite3_open(path.c_str(), &m_pDb);
    }
    // device-only (ОПУЩЕНО off-device): проба `select count(*) from tb_DcmWorklist` без ключа;
    // при ошибке sqlite3_key(m_pDb, "SONOSCOPE_X2000_KEY", 19) + повтор пробы (SQLCipher-unlock).
    sqlite3_mutex_leave(m_pMutex);
    return rc;
}

int KDbSqlite::Close()
{
    sqlite3_mutex_enter(m_pMutex);
    int rc = 0;
    while (m_pDb) {
        rc = sqlite3_close(m_pDb);
        if (rc == SQLITE_BUSY) {          // незакрытые statements → sleep + повтор
            sqlite3_sleep(100);
            continue;
        }
        if (rc == 0)
            m_pDb = nullptr;
        else
            break;                        // реф. — лог + возврат кода ошибки
    }
    sqlite3_mutex_leave(m_pMutex);
    return rc;
}

bool KDbSqlite::IsOpen()
{
    sqlite3_mutex_enter(m_pMutex);
    const bool open = m_pDb != nullptr;
    sqlite3_mutex_leave(m_pMutex);
    return open;
}

int KDbSqlite::Exec(const char *sql)
{
    if (!sql)
        return -4102;   // реф. -0x1006 (нулевой sql)

    std::string s(sql);
    // реф. KDbStringOperation::ConvertCharactersetToUTF8(s) — у нас стаб→true (no-op); при false
    // реф. вернул бы -4097 (-0x1001).
    FilterLogSql(s);   // реф. стаб→true → лог SQL подавлен

    sqlite3_mutex_enter(m_pMutex);
    if (!m_pDb) {                          // реф. — LogPrintfEx "db not open" + код ошибки
        sqlite3_mutex_leave(m_pMutex);
        return SQLITE_ERROR;
    }
    char *errmsg = nullptr;
    int rc = sqlite3_exec(m_pDb, s.c_str(), nullptr, nullptr, &errmsg);
    while (rc == SQLITE_BUSY) {            // реф. ретрай BUSY
        sqlite3_sleep(100);
        sqlite3_free(errmsg);
        errmsg = nullptr;
        rc = sqlite3_exec(m_pDb, s.c_str(), nullptr, nullptr, &errmsg);
    }
    sqlite3_mutex_leave(m_pMutex);

    if (errmsg) {                          // реф. — m_strLastError = errmsg; sqlite3_free
        m_strLastError = errmsg;
        sqlite3_free(errmsg);
    }
    return rc;   // реф. при rc!=0 ремапит на внутр. константы; off-device — прямой sqlite rc
}

int KDbSqlite::InsertField(const std::string &table, const std::string &field)
{
    // реф. @0x447108: snprintf(buf, "alter table %s add %s varchar", table, field) → Exec.
    std::vector<char> buf(SQL_BUF_SIZE);
    snprintf(buf.data(), buf.size(), "alter table %s add %s varchar", table.c_str(), field.c_str());
    return Exec(buf.data());
}

int KDbSqlite::DeleteRecord(const std::string &table, const std::string &where)
{
    // реф. @0x447068: пустой where → "delete from %s"; иначе "delete from %s where %s".
    std::vector<char> buf(SQL_BUF_SIZE);
    if (where.empty())
        snprintf(buf.data(), buf.size(), "delete from %s", table.c_str());
    else
        snprintf(buf.data(), buf.size(), "delete from %s where %s", table.c_str(), where.c_str());
    return Exec(buf.data());
}

int KDbSqlite::GetFieldNameList(const std::string &table, std::set<std::string> &out)
{
    // реф. @0x446be0: sprintf("select * from %s", table) → prepare_v2 → column_name[i] в set.
    std::vector<char> buf(SQL_BUF_SIZE);
    snprintf(buf.data(), buf.size(), "select * from %s", table.c_str());

    sqlite3_stmt *stmt = nullptr;
    sqlite3_mutex_enter(m_pMutex);
    if (!m_pDb) {                          // реф. — лог "db not open" + код ошибки
        sqlite3_mutex_leave(m_pMutex);
        return SQLITE_ERROR;
    }
    int rc = sqlite3_prepare_v2(m_pDb, buf.data(), -1, &stmt, nullptr);
    while (rc == SQLITE_BUSY) {            // реф. ретрай BUSY
        sqlite3_sleep(100);
        rc = sqlite3_prepare_v2(m_pDb, buf.data(), -1, &stmt, nullptr);
    }
    sqlite3_mutex_leave(m_pMutex);

    if (rc != SQLITE_OK) {                 // реф. — finalize + лог + код ошибки
        if (stmt)
            sqlite3_finalize(stmt);
        return rc;
    }
    if (stmt) {
        const int n = sqlite3_column_count(stmt);
        for (int i = 0; i < n; ++i) {
            const char *name = sqlite3_column_name(stmt, i);
            out.insert(std::string(name ? name : ""));
        }
        sqlite3_finalize(stmt);
    }
    return rc;
}

int KDbSqlite::InsertRecord(const std::map<std::string, std::string> &fields,
                            const std::string &table)
{
    // реф. @0x447190: колонки таблицы через GetFieldNameList; в INSERT попадают ТОЛЬКО поля,
    // чей ключ реально есть в таблице; значения — sqlite3_snprintf("%Q").
    std::set<std::string> cols;
    int rc = GetFieldNameList(table, cols);
    if (rc != SQLITE_OK)
        return rc;

    std::string colList, valList;
    for (const auto &kv : fields) {
        if (cols.find(kv.first) == cols.end())   // ключа нет как колонки → пропуск
            continue;
        if (!colList.empty())
            colList += ",";
        colList += kv.first;
        if (!valList.empty())
            valList += ",";
        std::vector<char> vbuf(SQL_BUF_SIZE);
        sqlite3_snprintf(SQL_BUF_SIZE - 1, vbuf.data(), "%Q", kv.second.c_str());
        valList += vbuf.data();
    }

    char *sql = sqlite3_mprintf("insert into %s (%s) values(%s)",
                                table.c_str(), colList.c_str(), valList.c_str());
    if (!sql)                              // реф. — mprintf==null → -0x1005
        return -4101;
    rc = Exec(sql);
    sqlite3_free(sql);
    return rc;
}

int KDbSqlite::UpdateRecord(const std::map<std::string, std::string> &fields,
                            const std::string &table, const std::string &where)
{
    // реф. @0x447470: как InsertRecord, но SET-пары "col=%Q" (запятая между, не ведущая).
    std::set<std::string> cols;
    int rc = GetFieldNameList(table, cols);
    if (rc != SQLITE_OK)
        return rc;

    std::string setClause;
    for (const auto &kv : fields) {
        if (cols.find(kv.first) == cols.end())
            continue;
        if (!setClause.empty())
            setClause += ",";
        setClause += kv.first;
        setClause += "=";
        std::vector<char> vbuf(SQL_BUF_SIZE);
        sqlite3_snprintf(SQL_BUF_SIZE - 1, vbuf.data(), "%Q", kv.second.c_str());
        setClause += vbuf.data();
    }

    char *sql = where.empty()
        ? sqlite3_mprintf("update %s set %s", table.c_str(), setClause.c_str())
        : sqlite3_mprintf("update %s set %s where %s",
                          table.c_str(), setClause.c_str(), where.c_str());
    if (!sql)
        return -4101;
    rc = Exec(sql);
    sqlite3_free(sql);
    return rc;
}

int KDbSqlite::GetRecordsNumber(const std::string &table, const std::string &where) const
{
    // реф. @0x4466c8: count-запрос → sqlite3_get_table → strtol первой ячейки данных (azResult[ncol]).
    char buf[1024];
    if (where.empty())
        snprintf(buf, sizeof(buf) - 1, "select count(*) from %s", table.c_str());
    else
        snprintf(buf, sizeof(buf) - 1, "select count(*) from %s where %s",
                 table.c_str(), where.c_str());

    sqlite3_mutex_enter(m_pMutex);
    char **azResult = nullptr;
    int nrow = 0, ncol = 0;
    char *errmsg = nullptr;
    int result = 0;
    const int rc = sqlite3_get_table(m_pDb, buf, &azResult, &nrow, &ncol, &errmsg);
    if (rc == SQLITE_OK && azResult && azResult[ncol])
        result = static_cast<int>(strtol(azResult[ncol], nullptr, 10));   // первая ячейка данных
    if (azResult)
        sqlite3_free_table(azResult);
    if (errmsg)
        sqlite3_free(errmsg);
    sqlite3_mutex_leave(m_pMutex);
    return result;
}

int KDbSqlite::QuerySingleRecord(const std::string &table, const std::string &where,
                                 std::map<std::string, std::string> &out)
{
    // реф. @0x447758: select первой строки → map(колонка→значение). out НЕ очищается.
    std::vector<char> buf(SQL_BUF_SIZE);
    if (where.empty())
        snprintf(buf.data(), SQL_BUF_SIZE - 1, "select * from %s limit 1", table.c_str());
    else
        snprintf(buf.data(), SQL_BUF_SIZE - 1, "select * from %s where (%s) limit 1",
                 table.c_str(), where.c_str());

    sqlite3_mutex_enter(m_pMutex);
    char **az = nullptr;
    int nrow = 0, ncol = 0;
    char *errmsg = nullptr;
    int rc = sqlite3_get_table(m_pDb, buf.data(), &az, &nrow, &ncol, &errmsg);
    while (rc == SQLITE_BUSY) {            // реф. — второй get_table (ретрай BUSY)
        sqlite3_sleep(100);
        if (az) { sqlite3_free_table(az); az = nullptr; }
        if (errmsg) { sqlite3_free(errmsg); errmsg = nullptr; }
        rc = sqlite3_get_table(m_pDb, buf.data(), &az, &nrow, &ncol, &errmsg);
    }
    if (rc == SQLITE_OK && az && nrow >= 1) {
        for (int i = 0; i < ncol; ++i) {
            const char *col = az[i];               // заголовки колонок [0..ncol)
            const char *val = az[ncol + i];        // первая строка данных [ncol..2*ncol)
            out[col ? col : ""] = val ? val : "";
        }
    }
    if (az)
        sqlite3_free_table(az);
    if (errmsg)
        sqlite3_free(errmsg);
    sqlite3_mutex_leave(m_pMutex);
    return rc;
}
