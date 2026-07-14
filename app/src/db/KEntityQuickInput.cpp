#include "db/KEntityQuickInput.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDateTime>
#include <QDebug>

QString KEntityQuickInput::TableName(Kind k)
{
    switch (k) {
    case Patient:     return "tb_QuickInputPatient";
    case Doctor:      return "tb_QuickInputDoctor";
    case Applicant:   return "tb_QuickInputApplicant";
    case ReportTitle: return "tb_QuickInputReportTitle";
    }
    return "tb_QuickInputPatient";
}

KEntityQuickInput::KEntityQuickInput(Kind kind, const QString &connectionName)
    : kind_(kind), table_(TableName(kind)), conn_(connectionName)
{
}

bool KEntityQuickInput::CreateTable() const
{
    QSqlQuery q(QSqlDatabase::database(conn_));
    // Колонки из реверса: value (ключ), Count (частота), date.
    const bool ok = q.exec(QString(
        "CREATE TABLE IF NOT EXISTS %1 ("
        "value TEXT PRIMARY KEY, Count INTEGER DEFAULT 0, date TEXT)").arg(table_));
    if (!ok) qWarning() << "KEntityQuickInput::CreateTable:" << q.lastError().text();
    return ok;
}

bool KEntityQuickInput::SaveData(const QString &value, const QString &date)
{
    if (value.trimmed().isEmpty())
        return false;
    const QString d = date.isEmpty()
        ? QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") : date;
    // Инкремент частоты при повторе (реф. SaveData): UPSERT через INSERT OR IGNORE + UPDATE.
    QSqlQuery q(QSqlDatabase::database(conn_));
    q.prepare(QString("INSERT OR IGNORE INTO %1 (value, Count, date) VALUES (?, 0, ?)").arg(table_));
    q.addBindValue(value);
    q.addBindValue(d);
    if (!q.exec()) { qWarning() << "QI insert:" << q.lastError().text(); return false; }
    q.prepare(QString("UPDATE %1 SET Count = Count + 1, date = ? WHERE value = ?").arg(table_));
    q.addBindValue(d);
    q.addBindValue(value);
    return q.exec();
}

QList<KQIDEntity> KEntityQuickInput::GetAllEntity() const
{
    QList<KQIDEntity> out;
    QSqlQuery q(QSqlDatabase::database(conn_));
    // По убыванию частоты, затем по свежести (ранжирование предложений).
    if (!q.exec(QString("SELECT value, Count, date FROM %1 ORDER BY Count DESC, date DESC").arg(table_)))
        return out;
    while (q.next())
        out.append({q.value(0).toString(), q.value(1).toInt(), q.value(2).toString()});
    return out;
}

QList<KQIDEntity> KEntityQuickInput::GetEntity(const QString &prefix) const
{
    QList<KQIDEntity> out;
    QSqlQuery q(QSqlDatabase::database(conn_));
    q.prepare(QString("SELECT value, Count, date FROM %1 WHERE value LIKE ? "
                      "ORDER BY Count DESC, date DESC").arg(table_));
    q.addBindValue(prefix + "%");
    if (!q.exec())
        return out;
    while (q.next())
        out.append({q.value(0).toString(), q.value(1).toInt(), q.value(2).toString()});
    return out;
}

bool KEntityQuickInput::DeleteSelf(const QString &value)
{
    QSqlQuery q(QSqlDatabase::database(conn_));
    q.prepare(QString("DELETE FROM %1 WHERE value = ?").arg(table_));
    q.addBindValue(value);
    return q.exec();
}

int KEntityQuickInput::GetEntityNumber() const
{
    QSqlQuery q(QSqlDatabase::database(conn_));
    if (q.exec(QString("SELECT count(*) FROM %1").arg(table_)) && q.next())
        return q.value(0).toInt();
    return 0;
}
