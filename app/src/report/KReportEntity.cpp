#include "report/KReportEntity.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QVariant>
#include <QDebug>

const char *const KReportDBTableHandler::m_strEntityType = "Report";

namespace {

QString g_conn = QStringLiteral("endo_main");

// Порядок — как в реверсе KReportEntity::ConvertToMap.
const char *const kCols[] = {
    "ExamId", "ReportDate", "ExamFindings", "Diagnose", "DiseaseName",
    "SurgicalMethod", "SurgeryFinding", "Biopsy", "CustomField1", "CustomField2",
    "Suggest", "HPType", "AssistDoctor", "ExamImg", "Reserved1", "Reserved2",
};
// Индекс единственной целочисленной колонки (HPType) в kCols.
const int kIntCol = 11;
const int kColCount = int(sizeof(kCols) / sizeof(kCols[0]));

QString colList()
{
    QString s;
    for (int i = 0; i < kColCount; ++i) {
        if (i) s += ", ";
        s += QLatin1String(kCols[i]);
    }
    return s;
}

// Привязка значений в порядке kCols (HPType — целочисленный, на своём месте).
void bindEntity(QSqlQuery &q, const KReportEntity &e)
{
    const std::string vals[] = {
        e.ExamId, e.ReportDate, e.ExamFindings, e.Diagnose, e.DiseaseName,
        e.SurgicalMethod, e.SurgeryFinding, e.Biopsy, e.CustomField1,
        e.CustomField2, e.Suggest, std::string(), e.AssistDoctor, e.ExamImg,
        e.Reserved1, e.Reserved2,
    };
    for (int i = 0; i < kColCount; ++i) {
        if (i == kIntCol)
            q.addBindValue(e.HPType);
        else
            q.addBindValue(QString::fromStdString(vals[i]));
    }
}

} // namespace

std::map<std::string, std::string> KReportEntity::ConvertToMap() const
{
    std::map<std::string, std::string> m;
    // Реф.: поля, равные "INVALID_STRING", в карту НЕ попадают.
    const auto put = [&m](const char *k, const std::string &v) {
        if (v != "INVALID_STRING")
            m[k] = v;
    };
    put("ExamId", ExamId);                 put("ReportDate", ReportDate);
    put("ExamFindings", ExamFindings);     put("Diagnose", Diagnose);
    put("DiseaseName", DiseaseName);       put("SurgicalMethod", SurgicalMethod);
    put("SurgeryFinding", SurgeryFinding); put("Biopsy", Biopsy);
    put("CustomField1", CustomField1);     put("CustomField2", CustomField2);
    put("Suggest", Suggest);               put("AssistDoctor", AssistDoctor);
    put("ExamImg", ExamImg);               put("Reserved1", Reserved1);
    put("Reserved2", Reserved2);
    // Реф.: HPType выводится через snprintf("%d") и ПРОПУСКАЕТСЯ при -1.
    if (HPType != -1)
        m["HPType"] = std::to_string(HPType);
    return m;
}

void KReportEntity::ConvertToEntry(const std::map<std::string, std::string> &m)
{
    auto get = [&m](const char *k) -> std::string {
        const auto it = m.find(k);
        return it == m.end() ? std::string() : it->second;
    };
    ExamId = get("ExamId");                 ReportDate = get("ReportDate");
    ExamFindings = get("ExamFindings");     Diagnose = get("Diagnose");
    DiseaseName = get("DiseaseName");
    SurgicalMethod = get("SurgicalMethod"); SurgeryFinding = get("SurgeryFinding");
    Biopsy = get("Biopsy");                 CustomField1 = get("CustomField1");
    CustomField2 = get("CustomField2");     Suggest = get("Suggest");
    AssistDoctor = get("AssistDoctor");     ExamImg = get("ExamImg");
    Reserved1 = get("Reserved1");           Reserved2 = get("Reserved2");
    const std::string hp = get("HPType");
    try {
        HPType = hp.empty() ? -1 : std::stoi(hp, nullptr, 10);
    } catch (const std::exception &) {
        HPType = -1;
    }
}

void KReportDBTableHandler::SetConnectionName(const QString &connectionName)
{
    g_conn = connectionName;
}

bool KReportDBTableHandler::CreateTable(const QString &connectionName)
{
    g_conn = connectionName;
    QSqlQuery q(QSqlDatabase::database(connectionName));
    // Реф. имя таблицы — РОВНО "Report" (без префикса tb_, в отличие от нашего
    // более раннего KEntityReport с tb_Report).
    QString ddl = QStringLiteral("CREATE TABLE IF NOT EXISTS Report (");
    for (int i = 0; i < kColCount; ++i) {
        if (i) ddl += ", ";
        ddl += QLatin1String(kCols[i]);
        ddl += (i == kIntCol) ? " INTEGER" : " TEXT";
    }
    ddl += ", PRIMARY KEY (ExamId))";
    if (!q.exec(ddl)) {
        qWarning() << "KReportDBTableHandler::CreateTable:" << q.lastError().text();
        return false;
    }
    return true;
}

int KReportDBTableHandler::AddNewReportEntity(const KReportEntity &e)
{
    QSqlQuery q(QSqlDatabase::database(g_conn));
    QString marks;
    for (int i = 0; i < kColCount; ++i) marks += (i ? ", ?" : "?");
    q.prepare("INSERT OR REPLACE INTO Report (" + colList() + ") VALUES (" + marks + ")");
    bindEntity(q, e);
    if (!q.exec()) {
        qWarning() << "KReportDBTableHandler::AddNewReportEntity:" << q.lastError().text();
        return -1;
    }
    return 0;
}

int KReportDBTableHandler::UpdateReportEntity(const std::string &examId, const KReportEntity &e)
{
    QSqlQuery q(QSqlDatabase::database(g_conn));
    QString sets;
    for (int i = 0; i < kColCount; ++i) {
        if (i) sets += ", ";
        sets += QLatin1String(kCols[i]);
        sets += "=?";
    }
    q.prepare("UPDATE Report SET " + sets + " WHERE ExamId=?");
    bindEntity(q, e);
    q.addBindValue(QString::fromStdString(examId));
    if (!q.exec()) {
        qWarning() << "KReportDBTableHandler::UpdateReportEntity:" << q.lastError().text();
        return -1;
    }
    return 0;
}

int KReportDBTableHandler::GetEntity(const std::string &examId, KReportEntity &out)
{
    QSqlQuery q(QSqlDatabase::database(g_conn));
    q.prepare("SELECT " + colList() + " FROM Report WHERE ExamId=?");
    q.addBindValue(QString::fromStdString(examId));
    if (!q.exec() || !q.next())
        return -1;
    std::map<std::string, std::string> m;
    for (int i = 0; i < kColCount; ++i)
        m[kCols[i]] = q.value(i).toString().toStdString();
    out.ConvertToEntry(m);
    return 0;
}

int KReportDBTableHandler::DeleteEntity(const std::string &examId)
{
    QSqlQuery q(QSqlDatabase::database(g_conn));
    q.prepare("DELETE FROM Report WHERE ExamId=?");
    q.addBindValue(QString::fromStdString(examId));
    return q.exec() ? 0 : -1;
}

int KReportDBTableHandler::DeleteEntites(const std::vector<std::string> & /*ids*/)
{
    // Реф.: тело не реализовано — сразу код «не поддерживается».
    return ERR_NOT_SUPPORT;
}

int KReportDBTableHandler::GetRecordNumber()
{
    QSqlQuery q(QSqlDatabase::database(g_conn));
    if (q.exec("SELECT count(*) FROM Report") && q.next())
        return q.value(0).toInt();
    return -1;
}

int KReportDBTableHandler::GetPageRecordFromDb(
    const std::map<std::string, std::string> & /*query*/,
    std::vector<std::map<std::string, std::string>> &out)
{
    // Реф.: карта запроса содержит ключ "query_fields" со значением "*".
    QSqlQuery q(QSqlDatabase::database(g_conn));
    if (!q.exec("SELECT " + colList() + " FROM Report"))
        return -1;
    while (q.next()) {
        std::map<std::string, std::string> m;
        for (int i = 0; i < kColCount; ++i)
            m[kCols[i]] = q.value(i).toString().toStdString();
        out.push_back(m);
    }
    return 0;
}

int KReportDBTableHandler::GetQueryRecordNum(const std::map<std::string, std::string> &query)
{
    std::vector<std::map<std::string, std::string>> v;
    if (GetPageRecordFromDb(query, v) != 0)
        return -1;
    return int(v.size());
}

int KReportDBTableHandler::GetAllRecordMainKey(std::vector<std::string> &out)
{
    std::map<std::string, std::string> query;
    query["query_fields"] = "*";
    std::vector<std::map<std::string, std::string>> v;
    if (GetPageRecordFromDb(query, v) != 0)
        return -1;
    for (const auto &m : v) {
        const auto it = m.find("ExamId");
        if (it != m.end())
            out.push_back(it->second);
    }
    return 0;
}
