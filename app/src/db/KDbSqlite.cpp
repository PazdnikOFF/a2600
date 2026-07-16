#include "db/KDbSqlite.h"

#include <cstdio>
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
