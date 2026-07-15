#include "db/KEntityService.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

KEntityService &KEntityService::GetInstance()
{
    static KEntityService inst;
    return inst;
}

bool KEntityService::ApplyEnvironment(const QString &connName)
{
    if (!QSqlDatabase::contains(connName))
        return false;
    QSqlQuery q(QSqlDatabase::database(connName));
    // 1:1 с KEntityService::SetEnvironment.
    const char *stmts[] = {
        "PRAGMA synchronous = NORMAL",
        "PRAGMA journal_mode = delete",
        "REINDEX",
    };
    for (const char *s : stmts) {
        if (!q.exec(s)) {
            qWarning() << "ApplyEnvironment:" << s << q.lastError().text();
            return false;
        }
    }
    return true;
}

QString KEntityService::BackupDatabase(const QString &dbPath, const QString &backupDir,
                                       const QString &stamp)
{
    if (!QFile::exists(dbPath))
        return QString();
    QDir().mkpath(backupDir);
    const QString base = QFileInfo(dbPath).completeBaseName();
    // "<base>_<YYYYMMDD_HHMMSS>.bak" (реф. Recover: копия с меткой времени + .bak).
    const QString bak = QDir(backupDir).absoluteFilePath(base + "_" + stamp + ".bak");
    QFile::remove(bak);
    if (!QFile::copy(dbPath, bak))
        return QString();
    return bak;
}

bool KEntityService::RecoverDatabase(const QString &bakPath, const QString &dbPath)
{
    if (!QFile::exists(bakPath))
        return false;
    QFile::remove(dbPath);
    return QFile::copy(bakPath, dbPath);
}
