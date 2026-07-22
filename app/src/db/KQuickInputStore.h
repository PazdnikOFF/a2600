#pragma once

#include <map>
#include <string>
#include <vector>

// DEVICE-STUB-сейм под generic-CRUD слой оригинала. В реф. хендлеры QuickInput ходят в
// `KEntityService::Instance().GetInnerEntityManage()` (@0x412180/0x412190) — движок поверх
// зашифрованной SQLite (SQLCipher, ключ SONOSCOPE_X2000_KEY), оперирующий
// `map<колонка,значение>` и именем таблицы. Здесь — тот же контракт как чистый интерфейс,
// чтобы хендлеры портировались 1:1, а бэкенд подменялся.
//
// Спека выборки (`spec`) повторяет KDbSqlite::QueryRecords: обычные ключи — равенство по
// колонке, спец-ключи "Where" (готовое условие), "Order" (order by), "Limit" (limit).
class KQuickInputStore
{
public:
    virtual ~KQuickInputStore() = default;

    // Реф. CreateEntity: INSERT. Возвращает присвоенный mKey (или -1 при ошибке).
    virtual int  CreateEntity(const std::string &table,
                              const std::map<std::string, std::string> &fields) = 0;
    // Реф. UpdateEntity: UPDATE ... WHERE mKey = key.
    virtual bool UpdateEntity(const std::string &table, const std::string &key,
                              const std::map<std::string, std::string> &fields) = 0;
    // Реф. DeleteEntity: DELETE ... WHERE mKey = key.
    virtual bool DeleteEntity(const std::string &table, const std::string &key) = 0;
    // Реф. GetEntityDetailList: SELECT по спеке.
    virtual bool GetEntityDetailList(const std::string &table,
                                     const std::map<std::string, std::string> &spec,
                                     std::vector<std::map<std::string, std::string>> &out) = 0;
    // Реф. GetRecordsNumber: SELECT count(*) [WHERE ...].
    virtual int  GetRecordsNumber(const std::string &table, const std::string &where) = 0;
};

// In-memory реализация для превью/self-test. Понимает УЗКОЕ подмножество SQL-условий,
// которое реально строят хендлеры QuickInput через KDbStrHandler:
//   "field = 'value'", "(A) and (B)", "(A) or (B)" — рекурсивно.
// Всё остальное считается невыполнимым условием (запись не проходит) — это стаб, а не БД.
class KQuickInputMemStore : public KQuickInputStore
{
public:
    int  CreateEntity(const std::string &table,
                      const std::map<std::string, std::string> &fields) override;
    bool UpdateEntity(const std::string &table, const std::string &key,
                      const std::map<std::string, std::string> &fields) override;
    bool DeleteEntity(const std::string &table, const std::string &key) override;
    bool GetEntityDetailList(const std::string &table,
                             const std::map<std::string, std::string> &spec,
                             std::vector<std::map<std::string, std::string>> &out) override;
    int  GetRecordsNumber(const std::string &table, const std::string &where) override;

    void Clear() { m_tables.clear(); m_nextKey = 1; }

private:
    using Row  = std::map<std::string, std::string>;
    using Rows = std::vector<Row>;

    std::map<std::string, Rows> m_tables;
    int m_nextKey = 1;
};
