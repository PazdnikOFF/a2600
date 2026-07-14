#include "db/KEntityManage.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

namespace {
const char *kConn = "endo_main";
}

KEntityManage &KEntityManage::Instance()
{
    static KEntityManage inst;
    return inst;
}

bool KEntityManage::OpenDb(const QString &dbPath)
{
    CloseDb();
    // На устройстве драйвер SQLCipher; в отладке — штатный QSQLITE.
    QSqlDatabase db = QSqlDatabase::contains(kConn)
        ? QSqlDatabase::database(kConn)
        : QSqlDatabase::addDatabase("QSQLITE", kConn);
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        qWarning() << "KEntityManage::OpenDb:" << db.lastError().text();
        return false;
    }
    opened_ = true;
    return createTables();
}

void KEntityManage::CloseDb()
{
    if (QSqlDatabase::contains(kConn)) {
        { QSqlDatabase::database(kConn).close(); }
        QSqlDatabase::removeDatabase(kConn);
    }
    opened_ = false;
}

bool KEntityManage::createTables()
{
    QSqlQuery q(QSqlDatabase::database(kConn));
    bool ok = q.exec(
        "CREATE TABLE IF NOT EXISTS tb_Patient ("
        "patientid TEXT PRIMARY KEY, PatientName TEXT, PatientSex TEXT, "
        "PatientAge TEXT, birthday TEXT)");
    ok = q.exec(
        "CREATE TABLE IF NOT EXISTS tb_Exam ("
        "AccessionNumber TEXT PRIMARY KEY, patientid TEXT, "
        "ExamDate TEXT, ExamTime TEXT, examfolder TEXT)") && ok;
    if (!ok) qWarning() << "createTables:" << q.lastError().text();
    return ok;
}

bool KEntityManage::AddPatientEntity(const PatientEntity &p)
{
    QSqlQuery q(QSqlDatabase::database(kConn));
    // реф.: insert into %s (%s) values(%s)
    q.prepare("INSERT OR REPLACE INTO tb_Patient "
              "(patientid, PatientName, PatientSex, PatientAge, birthday) "
              "VALUES (?, ?, ?, ?, ?)");
    q.addBindValue(p.patientId);
    q.addBindValue(p.patientName);
    q.addBindValue(p.patientSex);
    q.addBindValue(p.patientAge);
    q.addBindValue(p.birthday);
    if (!q.exec()) { qWarning() << "AddPatientEntity:" << q.lastError().text(); return false; }
    return true;
}

bool KEntityManage::GetPatientEntity(const QString &patientId, PatientEntity &out) const
{
    QSqlQuery q(QSqlDatabase::database(kConn));
    q.prepare("SELECT patientid, PatientName, PatientSex, PatientAge, birthday "
              "FROM tb_Patient WHERE patientid = ?");
    q.addBindValue(patientId);
    if (!q.exec() || !q.next()) return false;
    out.patientId   = q.value(0).toString();
    out.patientName = q.value(1).toString();
    out.patientSex  = q.value(2).toString();
    out.patientAge  = q.value(3).toString();
    out.birthday    = q.value(4).toString();
    return true;
}

QList<PatientEntity> KEntityManage::GetAllPatients() const
{
    QList<PatientEntity> res;
    QSqlQuery q(QSqlDatabase::database(kConn));
    if (!q.exec("SELECT patientid, PatientName, PatientSex, PatientAge, birthday FROM tb_Patient"))
        return res;
    while (q.next())
        res.append({q.value(0).toString(), q.value(1).toString(), q.value(2).toString(),
                    q.value(3).toString(), q.value(4).toString()});
    return res;
}

bool KEntityManage::AddExamEntity(const ExamEntity &e)
{
    QSqlQuery q(QSqlDatabase::database(kConn));
    q.prepare("INSERT OR REPLACE INTO tb_Exam "
              "(AccessionNumber, patientid, ExamDate, ExamTime, examfolder) "
              "VALUES (?, ?, ?, ?, ?)");
    q.addBindValue(e.accessionNumber);
    q.addBindValue(e.patientId);
    q.addBindValue(e.examDate);
    q.addBindValue(e.examTime);
    q.addBindValue(e.examFolder);
    if (!q.exec()) { qWarning() << "AddExamEntity:" << q.lastError().text(); return false; }
    return true;
}

QList<ExamEntity> KEntityManage::GetExamList(const QString &patientId) const
{
    QList<ExamEntity> res;
    QSqlQuery q(QSqlDatabase::database(kConn));
    q.prepare("SELECT AccessionNumber, patientid, ExamDate, ExamTime, examfolder "
              "FROM tb_Exam WHERE patientid = ?");
    q.addBindValue(patientId);
    if (!q.exec()) return res;
    while (q.next())
        res.append({q.value(0).toString(), q.value(1).toString(), q.value(2).toString(),
                    q.value(3).toString(), q.value(4).toString()});
    return res;
}
