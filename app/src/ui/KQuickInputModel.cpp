#include "KQuickInputModel.h"

#include "db/KQuickInputDbTableHandler.h"

// Селекторы LoadData (строки-ключи, НЕ имена SQL-таблиц).
static const char *kSelPatient   = "tb_QuickInputPatient";     // @0x863598
static const char *kSelDoctor    = "tb_QuickInputDoctor";      // @0x863560
static const char *kSelApplicant = "tb_QuickInputApplicant";   // @0x863528
static const char *kFieldName    = "PatientName";              // @0x83e490
static const char *kFieldId      = "PatientID";                // @0x83eb60

// Формат времени сущностей быстрого ввода (реф. литерал @0x83df48).
static const char *kTimeFormat = "yyyy-MM-dd hh:mm:ss";

namespace {

QDateTime ParseTime(const std::string &s)
{
    return QDateTime::fromString(QString::fromStdString(s), QLatin1String(kTimeFormat));
}

std::string NowTime()
{
    return QDateTime::currentDateTime().toString(QLatin1String(kTimeFormat)).toStdString();
}

}   // namespace

// ───────────────────────────── Стратегии ─────────────────────────────

void KQuickInputDataPatientName::GetData(std::vector<KComboBoxItem> &out, int count)
{
    // Реф. @0x5ac520 (сверено построчно): out.clear() → GetSortedData(entities, count) →
    // item{mKey = e.mKey, count = e.count, text = e.name}.
    out.clear();
    KQuickInputPatientDbTableHandler h;
    std::vector<KQIPEntity> rows;
    if (!h.GetSortedData(rows, count))
        return;
    for (const KQIPEntity &e : rows) {
        KComboBoxItem it;
        it.mKey  = e.mKey;
        it.count = e.count;
        it.time  = ParseTime(e.time);
        it.text  = e.name;
        out.push_back(it);
    }
}

int KQuickInputDataPatientName::SetData(const KComboBoxItem &item)
{
    // По аналогии с PatientID (@0x5ab038), но ключ ищется как ИМЯ. НЕ СВЕРЕНО ПОСТРОЧНО.
    KQuickInputPatientDbTableHandler h;
    int cnt = 0;
    KQIPEntity found;
    if (!h.IsExistEntity(item.text, std::string(), cnt, found))
        return -1;   // реф.: записи нет → ошибка; AddEntity здесь НЕ вызывается
    found.count += 1;
    found.time = NowTime();
    return h.UpdateEntity(std::to_string(found.mKey), found) ? 0 : -1;
}

void KQuickInputDataPatientID::GetData(std::vector<KComboBoxItem> &out, int count)
{
    // Сиблинг PatientName, но текст элемента — колонка id. НЕ СВЕРЕНО ПОСТРОЧНО.
    out.clear();
    KQuickInputPatientDbTableHandler h;
    std::vector<KQIPEntity> rows;
    if (!h.GetSortedData(rows, count))
        return;
    for (const KQIPEntity &e : rows) {
        KComboBoxItem it;
        it.mKey  = e.mKey;
        it.count = e.count;
        it.time  = ParseTime(e.time);
        it.text  = e.id;
        out.push_back(it);
    }
}

int KQuickInputDataPatientID::SetData(const KComboBoxItem &item)
{
    // Реф. @0x5ab038 (сверено): IsExistEntity(пустое имя, item.text как ID) → если нет,
    // -1; иначе count+1, time = сейчас, UpdateEntity(mKey).
    KQuickInputPatientDbTableHandler h;
    int cnt = 0;
    KQIPEntity found;
    if (!h.IsExistEntity(std::string(), item.text, cnt, found))
        return -1;
    found.count += 1;
    found.time = NowTime();
    return h.UpdateEntity(std::to_string(found.mKey), found) ? 0 : -1;
}

void KQuickInputDataDoctor::GetData(std::vector<KComboBoxItem> &out, int count)
{
    out.clear();
    KQuickInputDoctorDbTableHandler h;
    std::vector<KQIDEntity> rows;
    if (!h.GetSortedData(rows, count))
        return;
    for (const KQIDEntity &e : rows) {
        KComboBoxItem it;
        it.mKey  = e.mKey;
        it.count = e.count;
        it.time  = ParseTime(e.time);
        it.text  = e.name;
        out.push_back(it);
    }
}

int KQuickInputDataDoctor::SetData(const KComboBoxItem &item)
{
    // Реф. @0x5aa4e8 (сверено построчно).
    KQuickInputDoctorDbTableHandler h;
    int cnt = 0;
    KQIDEntity found;
    if (!h.IsExistEntity(item.text, cnt, found))
        return -1;
    found.count += 1;
    found.time = NowTime();
    return h.UpdateEntity(std::to_string(found.mKey), found) ? 0 : -1;
}

void KQuickInputDataApplicant::GetData(std::vector<KComboBoxItem> &out, int count)
{
    out.clear();
    KQuickInputApplicantDbTableHandler h;
    std::vector<KQIAEntity> rows;
    if (!h.GetSortedData(rows, count))
        return;
    for (const KQIAEntity &e : rows) {
        KComboBoxItem it;
        it.mKey  = e.mKey;
        it.count = e.count;
        it.time  = ParseTime(e.time);
        it.text  = e.name;
        out.push_back(it);
    }
}

int KQuickInputDataApplicant::SetData(const KComboBoxItem &item)
{
    KQuickInputApplicantDbTableHandler h;
    int cnt = 0;
    KQIAEntity found;
    if (!h.IsExistEntity(item.text, cnt, found))
        return -1;
    found.count += 1;
    found.time = NowTime();
    return h.UpdateEntity(std::to_string(found.mKey), found) ? 0 : -1;
}

// ─────────────────────────────── Модель ───────────────────────────────

KQuickInputModel::KQuickInputModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    // Реф. ctor @0x5a9928: пустой вектор, стратегии нет, лимит = 10.
}

void KQuickInputModel::LoadData(const std::string &table, const std::string &field, int limit)
{
    // Реф. @0x5a9ab8: лимит выставляется БЕЗУСЛОВНО, до выбора стратегии.
    m_limit = limit;

    if (table == kSelPatient && field == kFieldName)
        m_strategy = std::make_shared<KQuickInputDataPatientName>();
    else if (table == kSelPatient && field == kFieldId)
        m_strategy = std::make_shared<KQuickInputDataPatientID>();
    else if (table == kSelApplicant)
        m_strategy = std::make_shared<KQuickInputDataApplicant>();
    else if (table == kSelDoctor)
        m_strategy = std::make_shared<KQuickInputDataDoctor>();
    // иначе — стратегия остаётся прежней (реф. не сбрасывает её)

    if (!m_strategy)
        return;
    beginResetModel();
    m_strategy->GetData(m_items, m_limit);
    endResetModel();
}

int KQuickInputModel::SaveData(const KComboBoxItem &item)
{
    // Реф. @0x5a9e68.
    if (!m_strategy)
        return -1;
    if (m_strategy->SetData(item) != 0)
        return -1;
    beginResetModel();
    m_strategy->GetData(m_items, m_limit);
    endResetModel();
    return 0;
}

void KQuickInputModel::AllDataChanged()
{
    const int n = rowCount();
    if (n <= 0)
        return;
    emit dataChanged(index(0, 0), index(n - 1, 0));
}

QModelIndex KQuickInputModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() || column != 0 || row < 0 || row >= rowCount())
        return QModelIndex();
    return createIndex(row, column);   // internalId == 0 (реф. проверяет именно это)
}

QModelIndex KQuickInputModel::parent(const QModelIndex &) const
{
    return QModelIndex();   // реф.: плоская модель, родителя нет
}

int KQuickInputModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    const int n = static_cast<int>(m_items.size());
    return m_limit < n ? m_limit : n;   // реф.: min(size, limit)
}

int KQuickInputModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 1;   // реф.: всегда 1
}

QVariant KQuickInputModel::data(const QModelIndex &idx, int role) const
{
    // Реф. @0x5aa0a0: только column 0, internalId 0, row < min(size, limit) и роль
    // СТРОГО DisplayRole(0) или EditRole(2) — всё прочее (в т.ч. UserRole) → invalid.
    if (!idx.isValid() || idx.column() != 0 || idx.internalId() != 0)
        return QVariant();
    if (idx.row() < 0 || idx.row() >= rowCount())
        return QVariant();
    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();
    return QString::fromStdString(m_items[static_cast<size_t>(idx.row())].text);
}
