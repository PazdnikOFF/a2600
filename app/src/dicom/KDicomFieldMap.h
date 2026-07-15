#pragma once

#include <QString>
#include <QVector>

// Парсер XML-маппинга DICOM-датасет → колонки БД (реф. XmlParser + FieldMap).
// Файлы: presetdata/userpreset/dicom/WorklistFieldMap.xml (один <Record>) и
// Mpps{Create,Set}FieldMap.xml (несколько <Record> — PerformedProcedureStep/
// ProcedureCode/Series/…, с SubGroup/DatasetPath на уровне записи).
// Каждый <Field dbname="X" DatasetPath="DCM_.../DCM_..." format="date|time"/>
// связывает путь тега в датасете с именем колонки таблицы.
class KDicomFieldMap
{
public:
    struct Field {
        QString dbname;       // имя колонки БД
        QString datasetPath;  // путь тега(ов) в DICOM-датасете
        QString format;       // "date"/"time"/пусто
    };

    // Запись <Record type=".." SubGroup=".." DatasetPath="..">…</Record>.
    struct Record {
        QString type;             // атрибут type (PerformedProcedureStep/Series/…)
        QString subGroup;         // SubGroup (напр. DCM_ProcedureCodeSequence)
        QString datasetPath;      // DatasetPath на уровне записи (префикс/последовательность)
        QVector<Field> fields;
    };

    // Загрузить маппинг из XML.
    bool Load(const QString &xmlPath);

    // Все поля (плоско, все записи) — обратная совместимость.
    const QVector<Field> &Fields() const { return fields_; }
    // Записи с группировкой (реф. мульти-<Record> для MPPS).
    const QVector<Record> &Records() const { return records_; }
    int RecordCount() const { return records_.size(); }
    // Запись по type ("" если нет). found=nullptr допустим.
    Record RecordByType(const QString &type, bool *found = nullptr) const;

    // Уникальные имена колонок среди всех полей (для CREATE TABLE).
    QVector<QString> ColumnNames() const;
    // Тип первой записи (для одиночных карт вроде WorklistFieldMap).
    QString RecordType() const { return records_.isEmpty() ? QString() : records_.first().type; }
    bool IsEmpty() const { return fields_.isEmpty(); }

private:
    QVector<Field> fields_;
    QVector<Record> records_;
};
