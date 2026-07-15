#pragma once

#include <QString>

// Сервис обслуживания БД (реф. KEntityService, X-2600). НЕ запись-сущность, а
// инфраструктура: настройка окружения SQLite (SetEnvironment), бэкап/восстановление
// основной базы (Recover). Основной файл БД прошивки — HD-2000.dat.
//
// Off-device ядро (верифицируемо):
//   • ApplyEnvironment — PRAGMA synchronous=NORMAL + journal_mode=delete + REINDEX
//     (1:1 со строками SetEnvironment);
//   • BackupDatabase — копия базы в "<name>_<YYYYMMDD>_<HHMMSS>.bak" (реф. Recover);
//   • RecoverDatabase — восстановление из .bak-файла.
// HandleMsg/Subscribe (шина сообщений) — device/оркестрация, здесь не реализуются.
class KEntityService
{
public:
    static KEntityService &GetInstance();

    // Имя основного файла БД прошивки (реф.).
    static QString DatabaseFileName() { return "HD-2000.dat"; }

    // Применить PRAGMA-окружение к открытому соединению QSqlDatabase (реф. SetEnvironment).
    // connName — имя соединения (QSqlDatabase::database(connName)). true при успехе.
    static bool ApplyEnvironment(const QString &connName);

    // Бэкап файла БД в каталог: "<baseName>_<YYYYMMDD>_<HHMMSS>.bak".
    // Возвращает полный путь созданного бэкапа ("" при ошибке). stamp — метка времени
    // "YYYYMMDD_HHMMSS" (передаётся снаружи, т.к. время — не off-device-детерминировано).
    static QString BackupDatabase(const QString &dbPath, const QString &backupDir,
                                  const QString &stamp);

    // Восстановить БД из .bak (копирует bakPath → dbPath, перезаписывая). true при успехе.
    static bool RecoverDatabase(const QString &bakPath, const QString &dbPath);

private:
    KEntityService() = default;
};
