#pragma once

#include <map>
#include <string>

// Сущности словарей быстрого ввода (реф. KQIPEntity / KQIDEntity / KQIAEntity, X-2600).
// Три РАЗНЫХ класса (не один с enum-видом): пациент, врач, направивший. Каждая умеет
// ConvertToMap() — сериализацию в `map<колонка, значение>` для generic-CRUD слоя БД
// (реф. KEntityManage::CreateEntity/UpdateEntity(table, map)).
//
// Layout восстановлен дизасмом ConvertToMap/AddEntity/GetEntity/IsExistEntity + сверен по
// деструкторам (полный список std::string-полей). Имена колонок — строковые литералы
// бинарника (адреса в комментариях). У каждой сущности реф. имеет ещё 2 хвостовых
// std::string-поля, которые НЕ участвуют ни в ConvertToMap, ни в SQL — их назначение НЕ
// ВОССТАНОВЛЕНО, поэтому в порт они не переносятся (документируем как пробел).
//
// ВНИМАНИЕ: имена таблиц БЕЗ префикса `tb_` (см. KQuickInputDbTableHandler) — упрощённый
// слой `db/KEntityQuickInput.h` использует другую схему, это не одно и то же.

// Пациент (реф. KQIPEntity, sizeof 0xd8).
struct KQIPEntity
{
    int         mKey = -1;   // +0x00  колонка "mKey"     @0x862a38
    std::string id;          // +0x08  колонка "id"       @0x841a18
    std::string name;        // +0x28  колонка "name"
    int         sex = 0;     // +0x48  колонка "sex"      @0x862ac0
    std::string birthday;    // +0x50  колонка "birthday" @0x862ab0
    int         age = 0;     // +0x70  колонка "age"      @0x8823c8
    int         count = 0;   // +0x74  колонка "count"    @0x898738
    std::string time;        // +0x78  колонка "time"     @0x894968
    // +0x98, +0xb8 — ещё два std::string; в ConvertToMap/SQL не участвуют, роль НЕ ВОССТАНОВЛЕНА.

    std::map<std::string, std::string> ConvertToMap() const;
};

// Врач (реф. KQIDEntity).
struct KQIDEntity
{
    int         mKey = -1;   // +0x00  "mKey"
    std::string name;        // +0x08  "name"
    std::string account;     // +0x28  "account"  @0x862720
    int         count = 0;   // +0x48  "count"
    std::string time;        // +0x50  "time"
    // +0x70, +0x90 — не в SQL, роль НЕ ВОССТАНОВЛЕНА.

    std::map<std::string, std::string> ConvertToMap() const;
};

// Направивший (реф. KQIAEntity).
struct KQIAEntity
{
    int         mKey = -1;   // +0x00  "mKey"
    std::string name;        // +0x08  "name"
    int         count = 0;   // +0x28  "count"
    std::string time;        // +0x30  "time"
    // +0x50, +0x70 — не в SQL, роль НЕ ВОССТАНОВЛЕНА.

    std::map<std::string, std::string> ConvertToMap() const;
};
