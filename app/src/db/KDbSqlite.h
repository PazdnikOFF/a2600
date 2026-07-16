#pragma once

#include <sqlite3.h>

#include <string>

// Низкоуровневая обёртка SQLite (реф. KDbSqlite : IDatabase, X-2600) — фундамент под KEntity*.
// Реф. на RAW sqlite3 C API (+ SQLCipher). Off-device — на системном libsqlite3, БЕЗ шифрования:
// SQLCipher-«разблокировка» (проба `select count(*) from tb_DcmWorklist` без ключа → при ошибке
// sqlite3_key(m_pDb, "SONOSCOPE_X2000_KEY", 19) → повтор пробы) ОПУЩЕНА как device-only —
// plain libsqlite3 не имеет sqlite3_key. Реф. база IDatabase (чистый интерфейс, без данных) —
// off-device standalone; методы виртуальны как в реф.
//
// РЕАЛИЗОВАНО ЯДРО (Open/Close/IsOpen/Exec/GetLastErrorMsg/GetDbPath/SetLogEnabled/FilterLogSql,
// всё сверено дизасмом). CRUD/query (InsertRecord/UpdateRecord/DeleteRecord/InsertField/
// QueryRecords/QuerySingleRecord/GetFieldNameList/GetRecordsNumber) — отдельный заход (task-чип).
class KDbSqlite
{
public:
    KDbSqlite();
    virtual ~KDbSqlite();

    virtual int  Open(const std::string &path);   // sqlite3_open + ретрай BUSY; возврат rc
    virtual int  Close();                          // sqlite3_close + ретрай BUSY; null-ит handle
    virtual bool IsOpen();                          // (m_pDb != null) под mutex

    // реф.: charset→UTF8 (у нас стаб→true) + FilterLogSql + sqlite3_exec под mutex с ретраем BUSY;
    // errmsg → m_strLastError. null sql → -4102; charset-fail → -4097 (стаб не срабатывает).
    int Exec(const char *sql);
    int Exec(const std::string &sql) { return Exec(sql.c_str()); }

    // CRUD (реф. snprintf в буфер 0xa000 + Exec; SQL-литералы LOWERCASE, сверены дизасмом):
    int InsertField(const std::string &table, const std::string &field);   // "alter table %s add %s varchar"
    int DeleteRecord(const std::string &table, const std::string &where);  // "delete from %s [where %s]"

    std::string GetLastErrorMsg() const { return m_strLastError; }   // копия [0x08]
    std::string GetDbPath() const { return m_strDbPath; }            // копия [0x38]
    static void SetLogEnabled(bool on) { m_bIsLogOn = on; }          // статик m_bIsLogOn
    bool FilterLogSql(const std::string &) { return true; }          // реф. — стаб (mov w0,#1)

private:
    std::string    m_strLastError;      // +0x08
    sqlite3       *m_pDb = nullptr;      // +0x28
    sqlite3_mutex *m_pMutex = nullptr;   // +0x30
    std::string    m_strDbPath;         // +0x38

    static bool m_bIsLogOn;              // статический флаг логирования
};
