#pragma once

#include <QList>
#include <QString>

// Сущность/CRUD врача (реф. KEntityDoctor + KDoctorDBTableHandler, tb_Doctor, X-2600).
// Чистый SQLite (не UI/device), соединение endo_main, по паттерну KEntityPatient.
// ОТДЕЛЬНАЯ таблица-справочник врачей (account/пароль/счётчик) — НЕ дублирует
// tb_QuickInputDoctor (история автоввода, отдельные классы).
//
// 7 полей-строк реф. (ConvertToMap). id — технический PK (AUTOINCREMENT), account —
// бизнес-ключ. passwdLength/count семантически числовые (реф. stoi при чтении).
// 7-е поле реф. не декодировано полностью: колонка `time` ПОДТВЕРЖДЕНА через
// ORDER BY "time DESC, count DESC" (GetRecentUseAccount); альтернативный кандидат
// `name` не подтверждён.
struct KDoctorEntry {
    QString id;            // PK (пусто при вставке → AUTOINCREMENT)
    QString account;       // account (бизнес-ключ, поиск GetEntityByAccount)
    QString passwdLength;  // passwdLength (числовое)
    QString count;         // count (счётчик использований)
    QString time;          // time (последнее использование; для ORDER BY)
    QString reserved1;     // Reserved1
    QString reserved2;     // Reserved2
};

class KEntityDoctor
{
public:
    explicit KEntityDoctor(const QString &connectionName = "endo_main");

    bool CreateTable() const;
    bool CreateEntity(const KDoctorEntry &e);
    bool UpdateEntity(const QString &id, const KDoctorEntry &e);
    bool DeleteSelf(const QString &id);
    bool GetEntityDetail(const QString &id, KDoctorEntry &out) const;
    QList<KDoctorEntry> GetEntityDetailList() const;
    int  GetEntityNumber() const;

private:
    KDoctorEntry fromQuery(const class QSqlQuery &q) const;
    QString conn_;
};

// CRUD-хендлер (реф. KDoctorDBTableHandler) — тонкая обёртка над KEntityDoctor.
class KDoctorDBTableHandler
{
public:
    explicit KDoctorDBTableHandler(const QString &connectionName = "endo_main");

    int  GetRecordNumber() const;
    int  GetEntity(const QString &id, KDoctorEntry &out) const;   // 0 / -1
    bool AddNewEntity(const KDoctorEntry &e);
    bool UpdateEntity(const QString &id, const KDoctorEntry &e);
    bool DeleteEntity(const QString &id);
    QList<KDoctorEntry> GetAllEntities() const;
    QList<QString> GetAllAccount() const;                         // список account
    // Поиск по account (реф. BuildSimpleCondition("account","=",v)); 0/-1.
    int  GetEntityByAccount(const QString &account, KDoctorEntry &out) const;
    // Недавно использованные account: ORDER BY time DESC, count DESC (реф. + LIMIT N —
    // точное N из дизасма не извлечено; limit<0 → без ограничения).
    QList<QString> GetRecentUseAccount(int limit = -1) const;

private:
    KEntityDoctor ent_;
    QString conn_;
};
