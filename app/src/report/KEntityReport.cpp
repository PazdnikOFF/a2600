#include "report/KEntityReport.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

namespace {
const char *kConn = "endo_report";
}

KEntityReport &KEntityReport::Instance()
{
    static KEntityReport inst;
    return inst;
}

bool KEntityReport::OpenDb(const QString &dbPath)
{
    CloseDb();
    QSqlDatabase db = QSqlDatabase::contains(kConn)
        ? QSqlDatabase::database(kConn)
        : QSqlDatabase::addDatabase("QSQLITE", kConn);
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        qWarning() << "KEntityReport::OpenDb:" << db.lastError().text();
        return false;
    }
    return createTables();
}

void KEntityReport::CloseDb()
{
    if (QSqlDatabase::contains(kConn)) {
        { QSqlDatabase::database(kConn).close(); }
        QSqlDatabase::removeDatabase(kConn);
    }
}

bool KEntityReport::createTables()
{
    QSqlQuery q(QSqlDatabase::database(kConn));
    const bool ok = q.exec(
        "CREATE TABLE IF NOT EXISTS tb_Report ("
        "AccessionNumber TEXT PRIMARY KEY, TemplateName TEXT, ExamView TEXT, "
        "Diagnosis TEXT, DiseaseName TEXT, SurgicalMethod TEXT, "
        "SurgeryFinding TEXT, Suggestion TEXT, Biopsy TEXT, HP TEXT)");
    if (!ok) qWarning() << "KEntityReport::createTables:" << q.lastError().text();
    return ok;
}

bool KEntityReport::SaveReport(const ReportEntity &r)
{
    QSqlQuery q(QSqlDatabase::database(kConn));
    q.prepare("INSERT OR REPLACE INTO tb_Report "
              "(AccessionNumber, TemplateName, ExamView, Diagnosis, DiseaseName, "
              "SurgicalMethod, SurgeryFinding, Suggestion, Biopsy, HP) "
              "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    q.addBindValue(r.accessionNumber);
    q.addBindValue(r.templateName);
    q.addBindValue(r.examView);
    q.addBindValue(r.diagnosis);
    q.addBindValue(r.diseaseName);
    q.addBindValue(r.surgicalMethod);
    q.addBindValue(r.surgeryFinding);
    q.addBindValue(r.suggestion);
    q.addBindValue(r.biopsy);
    q.addBindValue(r.hp);
    if (!q.exec()) {
        qWarning() << "SaveReport:" << q.lastError().text();
        return false;
    }
    return true;
}

bool KEntityReport::GetReport(const QString &accessionNumber, ReportEntity &out) const
{
    QSqlQuery q(QSqlDatabase::database(kConn));
    q.prepare("SELECT TemplateName, ExamView, Diagnosis, DiseaseName, SurgicalMethod, "
              "SurgeryFinding, Suggestion, Biopsy, HP FROM tb_Report "
              "WHERE AccessionNumber = ?");
    q.addBindValue(accessionNumber);
    if (!q.exec() || !q.next())
        return false;
    out.accessionNumber = accessionNumber;
    out.templateName   = q.value(0).toString();
    out.examView       = q.value(1).toString();
    out.diagnosis      = q.value(2).toString();
    out.diseaseName    = q.value(3).toString();
    out.surgicalMethod = q.value(4).toString();
    out.surgeryFinding = q.value(5).toString();
    out.suggestion     = q.value(6).toString();
    out.biopsy         = q.value(7).toString();
    out.hp             = q.value(8).toString();
    return true;
}

bool KEntityReport::DeleteReport(const QString &accessionNumber)
{
    QSqlQuery q(QSqlDatabase::database(kConn));
    q.prepare("DELETE FROM tb_Report WHERE AccessionNumber = ?");
    q.addBindValue(accessionNumber);
    return q.exec();
}

int KEntityReport::GetReportNumber() const
{
    QSqlQuery q(QSqlDatabase::database(kConn));
    if (q.exec("SELECT count(*) FROM tb_Report") && q.next())
        return q.value(0).toInt();
    return 0;
}

namespace {
const char *kReportCols =
    "AccessionNumber, TemplateName, ExamView, Diagnosis, DiseaseName, "
    "SurgicalMethod, SurgeryFinding, Suggestion, Biopsy, HP";

ReportEntity reportFromQuery(const QSqlQuery &q)
{
    ReportEntity r;
    r.accessionNumber = q.value(0).toString();
    r.templateName    = q.value(1).toString();
    r.examView        = q.value(2).toString();
    r.diagnosis       = q.value(3).toString();
    r.diseaseName     = q.value(4).toString();
    r.surgicalMethod  = q.value(5).toString();
    r.surgeryFinding  = q.value(6).toString();
    r.suggestion      = q.value(7).toString();
    r.biopsy          = q.value(8).toString();
    r.hp              = q.value(9).toString();
    return r;
}
} // namespace

QList<ReportEntity> KEntityReport::GetPageRecord(int offset, int limit) const
{
    QList<ReportEntity> list;
    QSqlQuery q(QSqlDatabase::database(kConn));
    q.prepare("SELECT " + QString(kReportCols) +
              " FROM tb_Report ORDER BY AccessionNumber LIMIT ? OFFSET ?");
    q.addBindValue(limit);
    q.addBindValue(offset);
    if (!q.exec()) {
        qWarning() << "GetPageRecord:" << q.lastError().text();
        return list;
    }
    while (q.next()) list.append(reportFromQuery(q));
    return list;
}

QStringList KEntityReport::GetAllRecordMainKey() const
{
    QStringList keys;
    QSqlQuery q(QSqlDatabase::database(kConn));
    if (q.exec("SELECT AccessionNumber FROM tb_Report ORDER BY AccessionNumber"))
        while (q.next()) keys << q.value(0).toString();
    return keys;
}

int KEntityReport::GetQueryRecordNum(const QString &keyword) const
{
    QSqlQuery q(QSqlDatabase::database(kConn));
    if (keyword.isEmpty())
        return GetReportNumber();
    q.prepare("SELECT count(*) FROM tb_Report "
              "WHERE Diagnosis LIKE ? OR DiseaseName LIKE ?");
    const QString like = "%" + keyword + "%";
    q.addBindValue(like);
    q.addBindValue(like);
    if (q.exec() && q.next())
        return q.value(0).toInt();
    return 0;
}

QList<ReportEntity> KEntityReport::QueryPageRecord(const QString &keyword,
                                                   int offset, int limit) const
{
    if (keyword.isEmpty())
        return GetPageRecord(offset, limit);
    QList<ReportEntity> list;
    QSqlQuery q(QSqlDatabase::database(kConn));
    q.prepare("SELECT " + QString(kReportCols) +
              " FROM tb_Report WHERE Diagnosis LIKE ? OR DiseaseName LIKE ? "
              "ORDER BY AccessionNumber LIMIT ? OFFSET ?");
    const QString like = "%" + keyword + "%";
    q.addBindValue(like);
    q.addBindValue(like);
    q.addBindValue(limit);
    q.addBindValue(offset);
    if (!q.exec()) {
        qWarning() << "QueryPageRecord:" << q.lastError().text();
        return list;
    }
    while (q.next()) list.append(reportFromQuery(q));
    return list;
}
