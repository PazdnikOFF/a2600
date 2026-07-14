#include "db/KEntityExam.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

namespace {
const char *kCols = "ExamId, AccessionNumber, PatientId, ExamType, ExamDate, "
                    "ExamTime, ExamStatus, RegisterNumber, DrExamId, ExamDir";
}

KEntityExam::KEntityExam(const QString &connectionName)
    : conn_(connectionName)
{
}

bool KEntityExam::CreateTable() const
{
    QSqlQuery q(QSqlDatabase::database(conn_));
    const bool ok = q.exec(
        "CREATE TABLE IF NOT EXISTS tb_ExamList ("
        "ExamId TEXT PRIMARY KEY, AccessionNumber TEXT, PatientId TEXT, "
        "ExamType TEXT, ExamDate TEXT, ExamTime TEXT, ExamStatus INTEGER, "
        "RegisterNumber TEXT, DrExamId TEXT, ExamDir TEXT)");
    if (!ok) qWarning() << "KEntityExam::CreateTable:" << q.lastError().text();
    return ok;
}

ExamListEntity KEntityExam::fromQuery(const QSqlQuery &q) const
{
    ExamListEntity e;
    e.examId         = q.value(0).toString();
    e.accessionNumber= q.value(1).toString();
    e.patientId      = q.value(2).toString();
    e.examType       = q.value(3).toString();
    e.examDate       = q.value(4).toString();
    e.examTime       = q.value(5).toString();
    e.examStatus     = q.value(6).toInt();
    e.registerNumber = q.value(7).toString();
    e.drExamId       = q.value(8).toString();
    e.examDir        = q.value(9).toString();
    return e;
}

bool KEntityExam::CreateEntity(const ExamListEntity &e)
{
    QSqlQuery q(QSqlDatabase::database(conn_));
    q.prepare("INSERT OR REPLACE INTO tb_ExamList (" + QString(kCols) +
              ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    q.addBindValue(e.examId);         q.addBindValue(e.accessionNumber);
    q.addBindValue(e.patientId);      q.addBindValue(e.examType);
    q.addBindValue(e.examDate);       q.addBindValue(e.examTime);
    q.addBindValue(e.examStatus);     q.addBindValue(e.registerNumber);
    q.addBindValue(e.drExamId);       q.addBindValue(e.examDir);
    if (!q.exec()) { qWarning() << "CreateEntity:" << q.lastError().text(); return false; }
    return true;
}

bool KEntityExam::UpdateEntity(const ExamListEntity &e)
{
    QSqlQuery q(QSqlDatabase::database(conn_));
    q.prepare("UPDATE tb_ExamList SET AccessionNumber=?, PatientId=?, ExamType=?, "
              "ExamDate=?, ExamTime=?, ExamStatus=?, RegisterNumber=?, DrExamId=?, "
              "ExamDir=? WHERE ExamId=?");
    q.addBindValue(e.accessionNumber); q.addBindValue(e.patientId);
    q.addBindValue(e.examType);        q.addBindValue(e.examDate);
    q.addBindValue(e.examTime);        q.addBindValue(e.examStatus);
    q.addBindValue(e.registerNumber);  q.addBindValue(e.drExamId);
    q.addBindValue(e.examDir);         q.addBindValue(e.examId);
    return q.exec();
}

bool KEntityExam::DeleteSelf(const QString &examId)
{
    QSqlQuery q(QSqlDatabase::database(conn_));
    q.prepare("DELETE FROM tb_ExamList WHERE ExamId=?");
    q.addBindValue(examId);
    return q.exec();
}

bool KEntityExam::GetEntityDetail(const QString &examId, ExamListEntity &out) const
{
    QSqlQuery q(QSqlDatabase::database(conn_));
    q.prepare("SELECT " + QString(kCols) + " FROM tb_ExamList WHERE ExamId=?");
    q.addBindValue(examId);
    if (!q.exec() || !q.next())
        return false;
    out = fromQuery(q);
    return true;
}

QList<ExamListEntity> KEntityExam::GetEntityDetailList(const QString &patientId) const
{
    QList<ExamListEntity> list;
    QSqlQuery q(QSqlDatabase::database(conn_));
    q.prepare("SELECT " + QString(kCols) +
              " FROM tb_ExamList WHERE PatientId=? ORDER BY ExamDate DESC, ExamTime DESC");
    q.addBindValue(patientId);
    if (!q.exec()) return list;
    while (q.next()) list.append(fromQuery(q));
    return list;
}

QList<ExamListEntity> KEntityExam::GetEntitySummaryList() const
{
    QList<ExamListEntity> list;
    QSqlQuery q(QSqlDatabase::database(conn_));
    if (!q.exec("SELECT " + QString(kCols) + " FROM tb_ExamList ORDER BY ExamDate DESC"))
        return list;
    while (q.next()) list.append(fromQuery(q));
    return list;
}

int KEntityExam::GetEntityNumber() const
{
    QSqlQuery q(QSqlDatabase::database(conn_));
    if (q.exec("SELECT count(*) FROM tb_ExamList") && q.next())
        return q.value(0).toInt();
    return 0;
}

QString KEntityExam::GetLatestExamId() const
{
    QSqlQuery q(QSqlDatabase::database(conn_));
    // Последний по дате/времени (реф. GetLatestExamIdFromDb).
    if (q.exec("SELECT ExamId FROM tb_ExamList ORDER BY ExamDate DESC, ExamTime DESC LIMIT 1")
        && q.next())
        return q.value(0).toString();
    return QString();
}

QList<ExamListEntity> KEntityExam::GetPageRecord(int offset, int limit) const
{
    QList<ExamListEntity> list;
    QSqlQuery q(QSqlDatabase::database(conn_));
    q.prepare("SELECT " + QString(kCols) +
              " FROM tb_ExamList ORDER BY ExamDate DESC, ExamTime DESC LIMIT ? OFFSET ?");
    q.addBindValue(limit);
    q.addBindValue(offset);
    if (!q.exec()) return list;
    while (q.next()) list.append(fromQuery(q));
    return list;
}
