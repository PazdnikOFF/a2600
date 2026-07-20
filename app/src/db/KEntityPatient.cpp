#include "db/KEntityPatient.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QDebug>

namespace {
// Порядок — как в реф. KPatientEntry::ConvertToMap.
const char *kCols = "PatientID, PatientName, PatientSex, PatientBirthday, ApplicantDate, "
                    "Applicants, PlanDate, UserItem1, UserItem2, SickBedId, TelephoneNumber, "
                    "RegisterNumber, WorklistUID, PatientAge, ExamStatus, ExamType";
}

KEntityPatient::KEntityPatient(const QString &connectionName) : conn_(connectionName) {}

bool KEntityPatient::CreateTable() const
{
    QSqlQuery q(QSqlDatabase::database(conn_));
    const bool ok = q.exec(
        "CREATE TABLE IF NOT EXISTS tb_PatientList ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "PatientID TEXT, PatientName TEXT, PatientSex TEXT, PatientBirthday TEXT, "
        "ApplicantDate TEXT, Applicants TEXT, PlanDate TEXT, UserItem1 TEXT, UserItem2 TEXT, "
        "SickBedId TEXT, TelephoneNumber TEXT, RegisterNumber TEXT, WorklistUID TEXT, "
        "PatientAge TEXT, ExamStatus TEXT, ExamType TEXT)");
    if (!ok) qWarning() << "KEntityPatient::CreateTable:" << q.lastError().text();
    return ok;
}

KPatientEntry KEntityPatient::fromQuery(const QSqlQuery &q) const
{
    KPatientEntry e;
    e.id              = q.value(0).toString();
    e.patientID       = q.value(1).toString();
    e.patientName     = q.value(2).toString();
    e.patientSex      = q.value(3).toString();
    e.patientBirthday = q.value(4).toString();
    e.applicantDate   = q.value(5).toString();
    e.applicants      = q.value(6).toString();
    e.planDate        = q.value(7).toString();
    e.userItem1       = q.value(8).toString();
    e.userItem2       = q.value(9).toString();
    e.sickBedId       = q.value(10).toString();
    e.telephoneNumber = q.value(11).toString();
    e.registerNumber  = q.value(12).toString();
    e.worklistUID     = q.value(13).toString();
    e.patientAge      = q.value(14).toString();
    e.examStatus      = q.value(15).toString();
    e.examType        = q.value(16).toString();
    return e;
}

bool KEntityPatient::CreateEntity(const KPatientEntry &e)
{
    QSqlQuery q(QSqlDatabase::database(conn_));
    q.prepare("INSERT INTO tb_PatientList (" + QString(kCols) +
              ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    q.addBindValue(e.patientID);      q.addBindValue(e.patientName);
    q.addBindValue(e.patientSex);     q.addBindValue(e.patientBirthday);
    q.addBindValue(e.applicantDate);  q.addBindValue(e.applicants);
    q.addBindValue(e.planDate);
    q.addBindValue(e.userItem1);      q.addBindValue(e.userItem2);
    q.addBindValue(e.sickBedId);      q.addBindValue(e.telephoneNumber);
    q.addBindValue(e.registerNumber); q.addBindValue(e.worklistUID);
    q.addBindValue(e.patientAge);     q.addBindValue(e.examStatus);
    q.addBindValue(e.examType);
    if (!q.exec()) { qWarning() << "KEntityPatient::CreateEntity:" << q.lastError().text(); return false; }
    return true;
}

bool KEntityPatient::UpdateEntity(const QString &id, const KPatientEntry &e)
{
    QSqlQuery q(QSqlDatabase::database(conn_));
    q.prepare("UPDATE tb_PatientList SET PatientID=?, PatientName=?, PatientSex=?, "
              "PatientBirthday=?, ApplicantDate=?, Applicants=?, PlanDate=?, UserItem1=?, "
              "UserItem2=?, SickBedId=?, TelephoneNumber=?, RegisterNumber=?, WorklistUID=?, "
              "PatientAge=?, ExamStatus=?, ExamType=? WHERE id=?");
    q.addBindValue(e.patientID);      q.addBindValue(e.patientName);
    q.addBindValue(e.patientSex);     q.addBindValue(e.patientBirthday);
    q.addBindValue(e.applicantDate);  q.addBindValue(e.applicants);
    q.addBindValue(e.planDate);
    q.addBindValue(e.userItem1);      q.addBindValue(e.userItem2);
    q.addBindValue(e.sickBedId);      q.addBindValue(e.telephoneNumber);
    q.addBindValue(e.registerNumber); q.addBindValue(e.worklistUID);
    q.addBindValue(e.patientAge);     q.addBindValue(e.examStatus);
    q.addBindValue(e.examType);
    q.addBindValue(id);
    if (!q.exec()) { qWarning() << "KEntityPatient::UpdateEntity:" << q.lastError().text(); return false; }
    return true;
}

bool KEntityPatient::DeleteSelf(const QString &id)
{
    QSqlQuery q(QSqlDatabase::database(conn_));
    q.prepare("DELETE FROM tb_PatientList WHERE id=?");
    q.addBindValue(id);
    if (!q.exec()) { qWarning() << "KEntityPatient::DeleteSelf:" << q.lastError().text(); return false; }
    return true;
}

bool KEntityPatient::GetEntityDetail(const QString &id, KPatientEntry &out) const
{
    QSqlQuery q(QSqlDatabase::database(conn_));
    q.prepare("SELECT id, " + QString(kCols) + " FROM tb_PatientList WHERE id=?");
    q.addBindValue(id);
    if (!q.exec() || !q.next())
        return false;
    out = fromQuery(q);
    return true;
}

QList<KPatientEntry> KEntityPatient::GetEntityDetailList() const
{
    QList<KPatientEntry> out;
    QSqlQuery q(QSqlDatabase::database(conn_));
    if (q.exec("SELECT id, " + QString(kCols) + " FROM tb_PatientList ORDER BY id"))
        while (q.next())
            out.append(fromQuery(q));
    return out;
}

int KEntityPatient::GetEntityNumber() const
{
    QSqlQuery q(QSqlDatabase::database(conn_));
    if (q.exec("SELECT COUNT(*) FROM tb_PatientList") && q.next())
        return q.value(0).toInt();
    return 0;
}

// ————— KPatientListDBTableHandler —————

KPatientListDBTableHandler::KPatientListDBTableHandler(const QString &connectionName)
    : ent_(connectionName) {}

int KPatientListDBTableHandler::GetRecordNumber() const { return ent_.GetEntityNumber(); }

int KPatientListDBTableHandler::GetEntity(const QString &id, KPatientEntry &out) const
{
    return ent_.GetEntityDetail(id, out) ? 0 : -1;   // реф. коды 0/-1
}

bool KPatientListDBTableHandler::AddNewPatientEntity(const KPatientEntry &e)
{
    return ent_.CreateEntity(e);
}

bool KPatientListDBTableHandler::UpdatePatientEntity(const QString &id, const KPatientEntry &e)
{
    return ent_.UpdateEntity(id, e);
}

bool KPatientListDBTableHandler::DeleteEntity(const QString &id)
{
    return ent_.DeleteSelf(id);
}

bool KPatientListDBTableHandler::DeleteEntites(const QList<QString> &ids)
{
    (void)ids;
    // Реф. — заглушка: тело возвращает глобальный int, НИЧЕГО не удаляет. Воспроизведено.
    return false;
}

bool KPatientListDBTableHandler::UpdateExamStatus(const QString &id, int status)
{
    KPatientEntry e;
    if (!ent_.GetEntityDetail(id, e))
        return false;
    e.examStatus = QString::number(status);   // реф. sprintf("%d", status)
    return ent_.UpdateEntity(id, e);
}

QList<KPatientEntry> KPatientListDBTableHandler::GetPageRecordFromDb() const
{
    return ent_.GetEntityDetailList();
}
