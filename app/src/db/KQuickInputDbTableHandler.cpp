#include "db/KQuickInputDbTableHandler.h"
#include "db/KDbStrHandler.h"

#include <cstdlib>

// Имена таблиц — литералы бинарника (статические инициализаторы
// _GLOBAL__sub_I_KQuickInput*DbTableHandler.cpp).
static const char *kTablePatient   = "QuickInputPatient";     // @0x862af8
static const char *kTableDoctor    = "QuickInputDoctor";      // @0x862a90
static const char *kTableApplicant = "QuickInputApplicant";   // @0x862a70

// Спец-ключи спеки выборки (те же, что у KDbSqlite::QueryRecords).
static const char *kKeyWhere = "Where";   // @0x840078
static const char *kKeyOrder = "Order";   // @0x8548f8
static const char *kKeyLimit = "Limit";   // @0x862748

// ORDER BY в GetSortedData всех трёх хендлеров.
static const char *kSortOrder = "time DESC, count DESC";   // @0x862750

// GetMatchDate: ORDER BY / LIMIT (сверено в KQuickInputPatientDbTableHandler::GetMatchDate).
static const char *kMatchOrder = "time DESC";   // @0x862ac8
static const char *kMatchLimit = "10";          // @0x85db70 — ровно вместимость _ListBuff

KQuickInputStore *KQuickInputDbTableHandlerBase::s_store = nullptr;

namespace {

std::string Field(const std::map<std::string, std::string> &row, const std::string &k)
{
    auto it = row.find(k);
    return it == row.end() ? std::string() : it->second;
}

int IntField(const std::map<std::string, std::string> &row, const std::string &k, int def = 0)
{
    auto it = row.find(k);
    return it == row.end() || it->second.empty() ? def : std::atoi(it->second.c_str());
}

// WHERE для GetMatchDate: «<колонка> LIKE '%<префикс>%'» (реф. собирает конкатенацией
// литералов " LIKE " @0x862a40, "'%" @0x862a48, "%'" @0x862a50).
std::string LikeCondition(const std::string &column, const std::string &prefix)
{
    return column + " LIKE '%" + KDbStrHandler::SqliteCharsEscape(prefix) + "%'";
}

}   // namespace

bool KQuickInputDbTableHandlerBase::DeleteEntity(const std::string &key)
{
    return Store() && Store()->DeleteEntity(m_table, key);
}

int KQuickInputDbTableHandlerBase::GetEntityNumber(const std::string &where)
{
    return Store() ? Store()->GetRecordsNumber(m_table, where) : 0;
}

std::map<std::string, std::string> KQuickInputDbTableHandlerBase::SortSpec(int limit) const
{
    std::map<std::string, std::string> spec;
    spec[kKeyOrder] = kSortOrder;
    if (limit > 0)
        spec[kKeyLimit] = std::to_string(limit);
    return spec;
}

// ─────────────────────────────── Пациенты ───────────────────────────────

KQuickInputPatientDbTableHandler::KQuickInputPatientDbTableHandler()
    : KQuickInputDbTableHandlerBase(kTablePatient)
{
}

int KQuickInputPatientDbTableHandler::AddEntity(KQIPEntity &e)
{
    if (!Store())
        return -1;
    const int key = Store()->CreateEntity(m_table, e.ConvertToMap());
    if (key >= 0)
        e.mKey = key;   // реф. отдаёт назначенный ключ обратно в сущность
    return key;
}

bool KQuickInputPatientDbTableHandler::UpdateEntity(const std::string &key, KQIPEntity &e)
{
    return Store() && Store()->UpdateEntity(m_table, key, e.ConvertToMap());
}

bool KQuickInputPatientDbTableHandler::GetEntity(
    const std::map<std::string, std::string> &cond, std::vector<KQIPEntity> &out)
{
    if (!Store())
        return false;
    std::vector<std::map<std::string, std::string>> rows;
    if (!Store()->GetEntityDetailList(m_table, cond, rows))
        return false;
    for (const auto &r : rows) {
        KQIPEntity e;
        e.mKey     = IntField(r, "mKey", -1);
        e.id       = Field(r, "id");
        e.name     = Field(r, "name");
        e.sex      = IntField(r, "sex");
        e.birthday = Field(r, "birthday");
        e.age      = IntField(r, "age");
        e.count    = IntField(r, "count");
        e.time     = Field(r, "time");
        out.push_back(e);
    }
    return true;
}

bool KQuickInputPatientDbTableHandler::GetAllEntity(std::vector<KQIPEntity> &out)
{
    return GetEntity(std::map<std::string, std::string>(), out);
}

bool KQuickInputPatientDbTableHandler::GetSortedData(std::vector<KQIPEntity> &out, int limit)
{
    return GetEntity(SortSpec(limit), out);
}

bool KQuickInputPatientDbTableHandler::IsExistEntity(const std::string &name,
                                                     const std::string &id, int &cnt,
                                                     KQIPEntity &out)
{
    // Реф. @0x4269f8: BuildOrCondition(BuildSimpleCondition("name",..), BuildSimpleCondition("id",..)).
    const std::string where = KDbStrHandler::BuildOrCondition(
        KDbStrHandler::BuildSimpleCondition("name", name, "="),
        KDbStrHandler::BuildSimpleCondition("id", id, "="));
    cnt = GetEntityNumber(where);
    if (cnt <= 0)
        return false;
    std::map<std::string, std::string> spec;
    spec[kKeyWhere] = where;
    spec[kKeyLimit] = "1";
    std::vector<KQIPEntity> rows;
    if (!GetEntity(spec, rows) || rows.empty())
        return false;
    out = rows.front();
    return true;
}

bool KQuickInputPatientDbTableHandler::GetMatchDate(const std::string &prefix, _ListBuff &buff)
{
    // Реф. @0x427008: WHERE «id LIKE '%prefix%'» (колонка "id" @0x841a18), Limit "10",
    // Order "time DESC". Заполняются ТОЛЬКО Id/Name/DoB — Gender/Age остаются от Clear
    // (2 и 0 соответственно), реф. их здесь не трогает.
    std::map<std::string, std::string> spec;
    spec[kKeyWhere] = LikeCondition("id", prefix);
    spec[kKeyLimit] = kMatchLimit;
    spec[kKeyOrder] = kMatchOrder;

    std::vector<KQIPEntity> rows;
    if (!GetEntity(spec, rows))
        return false;
    int n = 0;
    for (const KQIPEntity &e : rows) {
        if (n >= _ListBuff::kMaxItems)
            break;
        buff.Id[n]   = QString::fromStdString(e.id);
        buff.Name[n] = QString::fromStdString(e.name);
        buff.DoB[n]  = QString::fromStdString(e.birthday);
        ++n;
    }
    return true;
}

// ─────────────────────────────── Врачи ───────────────────────────────

KQuickInputDoctorDbTableHandler::KQuickInputDoctorDbTableHandler()
    : KQuickInputDbTableHandlerBase(kTableDoctor)
{
}

int KQuickInputDoctorDbTableHandler::AddEntity(KQIDEntity &e)
{
    if (!Store())
        return -1;
    const int key = Store()->CreateEntity(m_table, e.ConvertToMap());
    if (key >= 0)
        e.mKey = key;
    return key;
}

bool KQuickInputDoctorDbTableHandler::UpdateEntity(const std::string &key, KQIDEntity &e)
{
    return Store() && Store()->UpdateEntity(m_table, key, e.ConvertToMap());
}

bool KQuickInputDoctorDbTableHandler::GetEntity(
    const std::map<std::string, std::string> &cond, std::vector<KQIDEntity> &out)
{
    if (!Store())
        return false;
    std::vector<std::map<std::string, std::string>> rows;
    if (!Store()->GetEntityDetailList(m_table, cond, rows))
        return false;
    for (const auto &r : rows) {
        KQIDEntity e;
        e.mKey    = IntField(r, "mKey", -1);
        e.name    = Field(r, "name");
        e.account = Field(r, "account");
        e.count   = IntField(r, "count");
        e.time    = Field(r, "time");
        out.push_back(e);
    }
    return true;
}

bool KQuickInputDoctorDbTableHandler::GetAllEntity(std::vector<KQIDEntity> &out)
{
    return GetEntity(std::map<std::string, std::string>(), out);
}

bool KQuickInputDoctorDbTableHandler::GetSortedData(std::vector<KQIDEntity> &out, int limit)
{
    return GetEntity(SortSpec(limit), out);
}

bool KQuickInputDoctorDbTableHandler::IsExistEntity(const std::string &name, int &cnt,
                                                    KQIDEntity &out)
{
    // Реф. @0x422f60: ОДНО условие «name = '..'» (без OR — в отличие от пациента).
    const std::string where = KDbStrHandler::BuildSimpleCondition("name", name, "=");
    cnt = GetEntityNumber(where);
    if (cnt <= 0)
        return false;
    std::map<std::string, std::string> spec;
    spec[kKeyWhere] = where;
    spec[kKeyLimit] = "1";
    std::vector<KQIDEntity> rows;
    if (!GetEntity(spec, rows) || rows.empty())
        return false;
    out = rows.front();
    return true;
}

bool KQuickInputDoctorDbTableHandler::GetMatchDate(const std::string &prefix, _ListBuff &buff)
{
    // Реф. @0x423390: WHERE «name LIKE '%prefix%'» (колонка "name" @0x898600).
    // Слот Id НЕ пишется (⇒ попап показывает голое имя без « - » — см. SearchMatchItem).
    // Реф. дополнительно пишет слоты DoB (+0x2a8) и Gender (+0x280 шаг 4), но КАКИЕ поля
    // сущности туда идут — НЕ ВОССТАНОВЛЕНО, поэтому здесь они не заполняются.
    std::map<std::string, std::string> spec;
    spec[kKeyWhere] = LikeCondition("name", prefix);
    spec[kKeyLimit] = kMatchLimit;
    spec[kKeyOrder] = kMatchOrder;

    std::vector<KQIDEntity> rows;
    if (!GetEntity(spec, rows))
        return false;
    int n = 0;
    for (const KQIDEntity &e : rows) {
        if (n >= _ListBuff::kMaxItems)
            break;
        buff.Name[n] = QString::fromStdString(e.name);
        ++n;
    }
    return true;
}

// ───────────────────────────── Направившие ─────────────────────────────

KQuickInputApplicantDbTableHandler::KQuickInputApplicantDbTableHandler()
    : KQuickInputDbTableHandlerBase(kTableApplicant)
{
}

int KQuickInputApplicantDbTableHandler::AddEntity(KQIAEntity &e)
{
    if (!Store())
        return -1;
    const int key = Store()->CreateEntity(m_table, e.ConvertToMap());
    if (key >= 0)
        e.mKey = key;
    return key;
}

bool KQuickInputApplicantDbTableHandler::UpdateEntity(const std::string &key, KQIAEntity &e)
{
    return Store() && Store()->UpdateEntity(m_table, key, e.ConvertToMap());
}

bool KQuickInputApplicantDbTableHandler::GetEntity(
    const std::map<std::string, std::string> &cond, std::vector<KQIAEntity> &out)
{
    if (!Store())
        return false;
    std::vector<std::map<std::string, std::string>> rows;
    if (!Store()->GetEntityDetailList(m_table, cond, rows))
        return false;
    for (const auto &r : rows) {
        KQIAEntity e;
        e.mKey  = IntField(r, "mKey", -1);
        e.name  = Field(r, "name");
        e.count = IntField(r, "count");
        e.time  = Field(r, "time");
        out.push_back(e);
    }
    return true;
}

bool KQuickInputApplicantDbTableHandler::GetAllEntity(std::vector<KQIAEntity> &out)
{
    return GetEntity(std::map<std::string, std::string>(), out);
}

bool KQuickInputApplicantDbTableHandler::GetSortedData(std::vector<KQIAEntity> &out, int limit)
{
    return GetEntity(SortSpec(limit), out);
}

bool KQuickInputApplicantDbTableHandler::IsExistEntity(const std::string &name, int &cnt,
                                                       KQIAEntity &out)
{
    // Реф. @0x420100: как у врача — одно условие «name = '..'».
    const std::string where = KDbStrHandler::BuildSimpleCondition("name", name, "=");
    cnt = GetEntityNumber(where);
    if (cnt <= 0)
        return false;
    std::map<std::string, std::string> spec;
    spec[kKeyWhere] = where;
    spec[kKeyLimit] = "1";
    std::vector<KQIAEntity> rows;
    if (!GetEntity(spec, rows) || rows.empty())
        return false;
    out = rows.front();
    return true;
}

bool KQuickInputApplicantDbTableHandler::GetMatchDate(const std::string &prefix, _ListBuff &buff)
{
    // Реф. @0x420530: WHERE «name LIKE '%prefix%'», как у врача.
    std::map<std::string, std::string> spec;
    spec[kKeyWhere] = LikeCondition("name", prefix);
    spec[kKeyLimit] = kMatchLimit;
    spec[kKeyOrder] = kMatchOrder;

    std::vector<KQIAEntity> rows;
    if (!GetEntity(spec, rows))
        return false;
    int n = 0;
    for (const KQIAEntity &e : rows) {
        if (n >= _ListBuff::kMaxItems)
            break;
        buff.Name[n] = QString::fromStdString(e.name);
        ++n;
    }
    return true;
}
