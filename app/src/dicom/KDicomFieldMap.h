#pragma once

#include <QString>
#include <QVector>

// Парсер XML-маппинга DICOM-датасет → колонки БД (реф. XmlParser + FieldMap).
// Файлы: presetdata/userpreset/dicom/WorklistFieldMap.xml (и Mpps*FieldMap.xml).
// Каждый <Field dbname="X" DatasetPath="DCM_.../DCM_..." format="date|time"/>
// связывает путь тега в датасете с именем колонки таблицы (tb_DcmWorklist и т.п.).
class KDicomFieldMap
{
public:
    struct Field {
        QString dbname;       // имя колонки БД
        QString datasetPath;  // путь тега(ов) в DICOM-датасете
        QString format;       // "date"/"time"/пусто
    };

    // Загрузить маппинг из XML (реф. запись <Record type="...">).
    bool Load(const QString &xmlPath);

    const QVector<Field> &Fields() const { return fields_; }
    // Уникальные имена колонок (для CREATE TABLE).
    QVector<QString> ColumnNames() const;
    QString RecordType() const { return recordType_; }
    bool IsEmpty() const { return fields_.isEmpty(); }

private:
    QVector<Field> fields_;
    QString recordType_;
};
