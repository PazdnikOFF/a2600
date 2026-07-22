#pragma once

#include <QString>
#include <QList>

// Словари автозаполнения быстрого ввода — УПРОЩЁННЫЙ off-device слой на Qt5::Sql
// (одна колонка-значение + частота + дата), 4 таблицы tb_QuickInput{Patient,Doctor,
// Applicant,ReportTitle}. Используется self-test-режимом `quickinput`.
//
// ⚠️ ЭТО НЕ РЕФЕРЕНСНАЯ СХЕМА. Точный порт живёт рядом, в `db/KQuickInputEntity.h` +
// `db/KQuickInputDbTableHandler.h`: там три РАЗНЫЕ сущности (KQIPEntity/KQIDEntity/
// KQIAEntity) со своими наборами колонок и таблицы БЕЗ префикса `tb_`
// (QuickInputPatient/QuickInputDoctor/QuickInputApplicant). Прежний плейсхолдер
// назывался `KQIDEntity` и занимал реф.-имя при несовпадающих полях — переименован
// в `KQuickInputRow`, чтобы освободить имя точному порту.

// Строка упрощённого словаря (НЕ реф. структура).
struct KQuickInputRow {
    QString value;
    int     count = 0;
    QString date;
};

class KEntityQuickInput
{
public:
    // Тип словаря → имя таблицы (реф. KQuickInput*DbTableHandler).
    enum Kind { Patient, Doctor, Applicant, ReportTitle };
    static QString TableName(Kind k);

    // Работает на соединении, открытом KEntityManage (одна БД пациентов).
    explicit KEntityQuickInput(Kind kind, const QString &connectionName = "endo_main");

    bool CreateTable() const;                       // CREATE TABLE IF NOT EXISTS
    // Сохранить значение (реф. SaveData): вставить или инкремент Count + обновить date.
    bool SaveData(const QString &value, const QString &date = QString());
    // Все записи по убыванию частоты (реф. GetAllEntity — предложения).
    QList<KQuickInputRow> GetAllEntity() const;
    // Записи по префиксу (реф. GetEntity — фильтр автодополнения).
    QList<KQuickInputRow> GetEntity(const QString &prefix) const;
    bool DeleteSelf(const QString &value);          // реф. DeleteSelf
    int  GetEntityNumber() const;

private:
    Kind    kind_;
    QString table_;
    QString conn_;
};
