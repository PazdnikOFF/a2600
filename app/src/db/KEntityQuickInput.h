#pragma once

#include <QString>
#include <QList>

// Словари автозаполнения быстрого ввода (реф. KEntityQuickInput* + KQIDEntity +
// KQuickInput*DbTableHandler, X-2600). 4 таблицы tb_QuickInput{Patient,Doctor,
// Applicant,ReportTitle}: по мере ввода в форму пациента сохраняются значения и
// предлагаются при следующем вводе (ранжирование по частоте).
// Колонки (реф.): value (текст), Count (частота), date (последнее использование).

// Строка словаря (реф. KQIDEntity).
struct KQIDEntity {
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
    QList<KQIDEntity> GetAllEntity() const;
    // Записи по префиксу (реф. GetEntity — фильтр автодополнения).
    QList<KQIDEntity> GetEntity(const QString &prefix) const;
    bool DeleteSelf(const QString &value);          // реф. DeleteSelf
    int  GetEntityNumber() const;

private:
    Kind    kind_;
    QString table_;
    QString conn_;
};
