#include "db/KQuickInputReportTitle.h"

#include <QDate>

#include <cstdlib>

const char *KQuickInputReportTitleData::kInvalidTitle = "INVALID_STRING";   // @0x83df60

// Имена таблиц — реф. литералы, С префиксом `tb_` (в отличие от трёх обычных словарей).
static const char *kTableTitle1 = "tb_QuickInputReportTitle1";   // @0x862da8
static const char *kTableTitle2 = "tb_QuickInputReportTitle2";   // @0x862de0

// Колонки.
static const char *kColKey   = "Key";     // @0x83e338
static const char *kColTitle = "Title";   // @0x877ee8
static const char *kColCount = "Count";   // @0x877ef0
static const char *kColTime  = "Time";    // @0x862678

// Спец-ключи спеки (те же, что у KDbSqlite::QueryRecords).
static const char *kKeyWhere = "Where";   // @0x840078
static const char *kKeyLimit = "Limit";   // @0x862748

// Формат заглушки DoB в GetMatchDate (@0x85dc10).
static const char *kDateFormat = "yyyy-MM-dd";

KQuickInputStore *KQuickInputReportTitleDBTableHandlerBase::s_store = nullptr;

namespace {

std::string Field(const std::map<std::string, std::string> &row, const std::string &k)
{
    auto it = row.find(k);
    return it == row.end() ? std::string() : it->second;
}

int IntField(const std::map<std::string, std::string> &row, const std::string &k, int def)
{
    auto it = row.find(k);
    return it == row.end() || it->second.empty() ? def : std::atoi(it->second.c_str());
}

}   // namespace

std::map<std::string, std::string> KQuickInputReportTitleData::ConvertToMap() const
{
    // Реф. @0x55b180: три колонки, каждая — с обходом по сентинелу.
    std::map<std::string, std::string> m;
    if (Key != -1)
        m[kColKey] = std::to_string(Key);
    if (Title != kInvalidTitle)
        m[kColTitle] = Title;
    if (Count != -1)
        m[kColCount] = std::to_string(Count);
    // "Time" реф. НЕ пишет — поле только читается.
    return m;
}

void KQuickInputReportTitleData::FromRow(const std::map<std::string, std::string> &row)
{
    Key   = IntField(row, kColKey, -1);
    Title = Field(row, kColTitle);
    Count = IntField(row, kColCount, -1);
    Time  = Field(row, kColTime);
}

bool KQuickInputReportTitleDBTableHandlerBase::QueryRecordByOtherKey(
    const std::string &field, const std::string &value, KQuickInputReportTitleData &out)
{
    if (!Store() || field.empty())
        return false;
    std::map<std::string, std::string> spec;
    spec[kKeyWhere] = field + " = '" + value + "'";
    spec[kKeyLimit] = "1";

    std::vector<std::map<std::string, std::string>> rows;
    if (!Store()->GetEntityDetailList(m_table, spec, rows) || rows.empty())
        return false;
    out.FromRow(rows.front());
    return true;
}

bool KQuickInputReportTitleDBTableHandlerBase::GetMatchDate(const std::string &prefix,
                                                            _ListBuff &buff)
{
    // 1) Предзаполнение ВСЕХ 10 слотов — реф. делает это безусловно, до запроса.
    //    Gender = 1 (у обычных словарей ClearListBuffData ставит 2 — здесь своё значение).
    const QString today = QDate::currentDate().toString(QLatin1String(kDateFormat));
    for (int i = 0; i < _ListBuff::kMaxItems; ++i) {
        buff.Id[i].clear();
        buff.Name[i].clear();
        buff.Gender[i] = 1;
        buff.Age[i]    = 0;
        buff.DoB[i]    = today;
    }
    buff.Count = 0;

    if (!Store())
        return false;

    // 2) Спека: WHERE «title like '%<prefix>%'» (литералы @0x88b9a0 + @0x862a50), Limit 10.
    //    Имя колонки в условии — в НИЖНЕМ регистре (`title`), хотя ConvertToMap пишет
    //    "Title": SQLite нечувствителен к регистру идентификаторов. Факт сохранён как есть.
    std::map<std::string, std::string> spec;
    spec[kKeyWhere] = std::string("title like '%") + prefix + "%'";
    spec[kKeyLimit] = "10";

    std::vector<std::map<std::string, std::string>> rows;
    if (!Store()->GetEntityDetailList(m_table, spec, rows))
        return false;

    // 3) Реф.: Name[i] = Title (цель — массив buff+0x140, источник — поле +0x8 сущности).
    int n = 0;
    for (const auto &r : rows) {
        if (n >= _ListBuff::kMaxItems)
            break;
        KQuickInputReportTitleData e;
        e.FromRow(r);
        buff.Name[n] = QString::fromStdString(e.Title);
        ++n;
    }
    buff.Count = n;
    return true;
}

KQuickInputReportTitle1DBTableHandler::KQuickInputReportTitle1DBTableHandler()
    : KQuickInputReportTitleDBTableHandlerBase(kTableTitle1)
{
}

KQuickInputReportTitle2DBTableHandler::KQuickInputReportTitle2DBTableHandler()
    : KQuickInputReportTitleDBTableHandlerBase(kTableTitle2)
{
}
