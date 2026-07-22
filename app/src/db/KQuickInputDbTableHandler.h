#pragma once

#include "db/KQuickInputEntity.h"
#include "db/KQuickInputListBuff.h"
#include "db/KQuickInputStore.h"

#include <string>
#include <vector>

// Хендлеры таблиц словарей быстрого ввода (реф. KQuickInput{Patient,Doctor,Applicant}
// DbTableHandler, X-2600). Синглтонов НЕТ — конструкторы тривиальны (реф.
// KQuickInputPatientDbTableHandler::KQuickInputPatientDbTableHandler @0x424db0 — пустой
// `ret`); общий движок берётся из KEntityService::Instance().GetInnerEntityManage().
//
// DEVICE-STUB: вместо KEntityManage (SQLCipher, ключ SONOSCOPE_X2000_KEY) — инъектируемый
// KQuickInputStore (см. KQuickInputStore.h). Имя таблицы, набор колонок, порядок сортировки
// и построение WHERE — как в реф. (адреса литералов в .cpp).
//
// Имена таблиц (реф., БЕЗ префикса `tb_`): QuickInputPatient @0x862af8,
// QuickInputDoctor @0x862a90, QuickInputApplicant @0x862a70.
// Сортировка у всех трёх (GetSortedData): "time DESC, count DESC" (@0x862750) + Limit.

// Общая часть трёх хендлеров: имя таблицы + ссылка на движок.
class KQuickInputDbTableHandlerBase
{
public:
    explicit KQuickInputDbTableHandlerBase(std::string table) : m_table(std::move(table)) {}
    virtual ~KQuickInputDbTableHandlerBase() = default;

    // DEVICE-STUB инъекция общего движка (реф. — KEntityService::Instance()).
    static void SetStore(KQuickInputStore *store) { s_store = store; }
    static KQuickInputStore *Store() { return s_store; }

    const std::string &TableName() const { return m_table; }

    bool DeleteEntity(const std::string &key);                   // реф. DeleteEntity
    int  GetEntityNumber(const std::string &where = std::string());

protected:
    // Спека сортировки реф. GetSortedData: {"Order": "time DESC, count DESC", "Limit": n}.
    std::map<std::string, std::string> SortSpec(int limit) const;

    std::string m_table;
    static KQuickInputStore *s_store;
};

// Пациенты: колонки mKey, id, name, sex, birthday, age, count, time.
class KQuickInputPatientDbTableHandler : public KQuickInputDbTableHandlerBase
{
public:
    KQuickInputPatientDbTableHandler();

    int  AddEntity(KQIPEntity &e);                                        // реф. @0x425d90
    bool UpdateEntity(const std::string &key, KQIPEntity &e);             // реф. @0x425e10
    bool GetEntity(const std::map<std::string, std::string> &cond,
                   std::vector<KQIPEntity> &out);                         // реф. @0x424de0
    bool GetAllEntity(std::vector<KQIPEntity> &out);                      // реф. @0x425e98
    bool GetSortedData(std::vector<KQIPEntity> &out, int limit);          // реф. @0x426090
    // Реф. @0x4269f8: WHERE = (name = '..') or (id = '..'); cnt — число совпадений.
    bool IsExistEntity(const std::string &name, const std::string &id, int &cnt,
                       KQIPEntity &out);
    // Реф. @0x427008: id/name/birthday первых записей → _ListBuff (Gender/Age НЕ заполняются).
    bool GetMatchDate(const std::string &prefix, _ListBuff &buff);
};

// Врачи: колонки mKey, name, account, count, time.
class KQuickInputDoctorDbTableHandler : public KQuickInputDbTableHandlerBase
{
public:
    KQuickInputDoctorDbTableHandler();

    int  AddEntity(KQIDEntity &e);                                        // реф. @0x4228a0
    bool UpdateEntity(const std::string &key, KQIDEntity &e);             // реф. @0x422920
    bool GetEntity(const std::map<std::string, std::string> &cond,
                   std::vector<KQIDEntity> &out);                         // реф. @0x421cc8
    bool GetAllEntity(std::vector<KQIDEntity> &out);                      // реф. @0x4229a8
    bool GetSortedData(std::vector<KQIDEntity> &out, int limit);          // реф. @0x422ba0
    bool IsExistEntity(const std::string &name, int &cnt, KQIDEntity &out);   // реф. @0x422f60
    bool GetMatchDate(const std::string &prefix, _ListBuff &buff);        // реф. @0x423390
};

// Направившие: колонки mKey, name, count, time.
class KQuickInputApplicantDbTableHandler : public KQuickInputDbTableHandlerBase
{
public:
    KQuickInputApplicantDbTableHandler();

    int  AddEntity(KQIAEntity &e);                                        // реф. @0x41f028
    bool UpdateEntity(const std::string &key, KQIAEntity &e);             // реф. @0x41f0a8
    bool GetEntity(const std::map<std::string, std::string> &cond,
                   std::vector<KQIAEntity> &out);                         // реф. @0x41f130
    bool GetAllEntity(std::vector<KQIAEntity> &out);                      // реф. @0x41fb48
    bool GetSortedData(std::vector<KQIAEntity> &out, int limit);          // реф. @0x41fd40
    bool IsExistEntity(const std::string &name, int &cnt, KQIAEntity &out);   // реф. @0x420100
    bool GetMatchDate(const std::string &prefix, _ListBuff &buff);        // реф. @0x420530
};
