#include "db/KEntityDoctor.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QDebug>

namespace {
const char *kCols = "account, passwdLength, count, time, Reserved1, Reserved2";
}

KEntityDoctor::KEntityDoctor(const QString &connectionName) : conn_(connectionName) {}

bool KEntityDoctor::CreateTable() const
{
    QSqlQuery q(QSqlDatabase::database(conn_));
    // Типы DDL в реф. живут в KEntityManage (не в этом классе) — приняты: id INTEGER PK,
    // passwdLength/count INTEGER (для корректного числового ORDER BY count DESC), прочее TEXT.
    const bool ok = q.exec(
        "CREATE TABLE IF NOT EXISTS tb_Doctor ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "account TEXT, passwdLength INTEGER, count INTEGER, time TEXT, "
        "Reserved1 TEXT, Reserved2 TEXT)");
    if (!ok) qWarning() << "KEntityDoctor::CreateTable:" << q.lastError().text();
    return ok;
}

KDoctorEntry KEntityDoctor::fromQuery(const QSqlQuery &q) const
{
    KDoctorEntry e;
    e.id           = q.value(0).toString();
    e.account      = q.value(1).toString();
    e.passwdLength = q.value(2).toString();
    e.count        = q.value(3).toString();
    e.time         = q.value(4).toString();
    e.reserved1    = q.value(5).toString();
    e.reserved2    = q.value(6).toString();
    return e;
}

bool KEntityDoctor::CreateEntity(const KDoctorEntry &e)
{
    QSqlQuery q(QSqlDatabase::database(conn_));
    q.prepare("INSERT INTO tb_Doctor (" + QString(kCols) + ") VALUES (?, ?, ?, ?, ?, ?)");
    q.addBindValue(e.account);      q.addBindValue(e.passwdLength);
    q.addBindValue(e.count);        q.addBindValue(e.time);
    q.addBindValue(e.reserved1);    q.addBindValue(e.reserved2);
    if (!q.exec()) { qWarning() << "KEntityDoctor::CreateEntity:" << q.lastError().text(); return false; }
    return true;
}

bool KEntityDoctor::UpdateEntity(const QString &id, const KDoctorEntry &e)
{
    QSqlQuery q(QSqlDatabase::database(conn_));
    q.prepare("UPDATE tb_Doctor SET account=?, passwdLength=?, count=?, time=?, "
              "Reserved1=?, Reserved2=? WHERE id=?");
    q.addBindValue(e.account);      q.addBindValue(e.passwdLength);
    q.addBindValue(e.count);        q.addBindValue(e.time);
    q.addBindValue(e.reserved1);    q.addBindValue(e.reserved2);
    q.addBindValue(id);
    if (!q.exec()) { qWarning() << "KEntityDoctor::UpdateEntity:" << q.lastError().text(); return false; }
    return true;
}

bool KEntityDoctor::DeleteSelf(const QString &id)
{
    QSqlQuery q(QSqlDatabase::database(conn_));
    q.prepare("DELETE FROM tb_Doctor WHERE id=?");
    q.addBindValue(id);
    if (!q.exec()) { qWarning() << "KEntityDoctor::DeleteSelf:" << q.lastError().text(); return false; }
    return true;
}

bool KEntityDoctor::GetEntityDetail(const QString &id, KDoctorEntry &out) const
{
    QSqlQuery q(QSqlDatabase::database(conn_));
    q.prepare("SELECT id, " + QString(kCols) + " FROM tb_Doctor WHERE id=?");
    q.addBindValue(id);
    if (!q.exec() || !q.next())
        return false;
    out = fromQuery(q);
    return true;
}

QList<KDoctorEntry> KEntityDoctor::GetEntityDetailList() const
{
    QList<KDoctorEntry> out;
    QSqlQuery q(QSqlDatabase::database(conn_));
    if (q.exec("SELECT id, " + QString(kCols) + " FROM tb_Doctor ORDER BY id"))
        while (q.next())
            out.append(fromQuery(q));
    return out;
}

int KEntityDoctor::GetEntityNumber() const
{
    QSqlQuery q(QSqlDatabase::database(conn_));
    if (q.exec("SELECT COUNT(*) FROM tb_Doctor") && q.next())
        return q.value(0).toInt();
    return 0;
}

// ————— KDoctorDBTableHandler —————

KDoctorDBTableHandler::KDoctorDBTableHandler(const QString &connectionName)
    : ent_(connectionName), conn_(connectionName) {}

int KDoctorDBTableHandler::GetRecordNumber() const { return ent_.GetEntityNumber(); }

int KDoctorDBTableHandler::GetEntity(const QString &id, KDoctorEntry &out) const
{
    return ent_.GetEntityDetail(id, out) ? 0 : -1;
}

bool KDoctorDBTableHandler::AddNewEntity(const KDoctorEntry &e) { return ent_.CreateEntity(e); }

bool KDoctorDBTableHandler::UpdateEntity(const QString &id, const KDoctorEntry &e)
{
    return ent_.UpdateEntity(id, e);
}

bool KDoctorDBTableHandler::DeleteEntity(const QString &id) { return ent_.DeleteSelf(id); }

QList<KDoctorEntry> KDoctorDBTableHandler::GetAllEntities() const
{
    return ent_.GetEntityDetailList();
}

QList<QString> KDoctorDBTableHandler::GetAllAccount() const
{
    QList<QString> out;
    for (const KDoctorEntry &e : ent_.GetEntityDetailList())
        out.append(e.account);
    return out;
}

int KDoctorDBTableHandler::GetEntityByAccount(const QString &account, KDoctorEntry &out) const
{
    QSqlQuery q(QSqlDatabase::database(conn_));
    q.prepare("SELECT id, account, passwdLength, count, time, Reserved1, Reserved2 "
              "FROM tb_Doctor WHERE account=?");
    q.addBindValue(account);
    if (!q.exec() || !q.next())
        return -1;
    out.id = q.value(0).toString();
    out.account = q.value(1).toString();
    out.passwdLength = q.value(2).toString();
    out.count = q.value(3).toString();
    out.time = q.value(4).toString();
    out.reserved1 = q.value(5).toString();
    out.reserved2 = q.value(6).toString();
    return 0;
}

QList<QString> KDoctorDBTableHandler::GetRecentUseAccount(int limit) const
{
    QList<QString> out;
    QString sql = "SELECT account FROM tb_Doctor ORDER BY time DESC, count DESC";
    if (limit >= 0)
        sql += " LIMIT " + QString::number(limit);
    QSqlQuery q(QSqlDatabase::database(conn_));
    if (q.exec(sql))
        while (q.next())
            out.append(q.value(0).toString());
    return out;
}
