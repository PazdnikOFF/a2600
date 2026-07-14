#pragma once

#include <QString>
#include <QVector>
#include <QStringList>

// Тезаурус шаблонов диагнозов (реф. KThesaurusOpt, X-2600). Словари по типу
// эндоскопии в presetdata/syspreset/patient/thesaurus/<lang>/<Scope>.xml.
// Каждая <record>: diseagroup (группа), checkedItemname (тип осмотра), diseaname
// (диагноз), examfinding (описание находок), diagresult (заключение), grid (ID).
// Выбор шаблона автозаполняет поля отчёта (RT_DISEASE_NAME/SURGERY_FINDING/DIAGNOSIS).
class KThesaurusOpt
{
public:
    struct Record {
        QString diseaGroup;      // diseagroup
        QString checkedItemName; // checkedItemname
        QString diseaName;       // diseaname
        QString examFinding;     // examfinding
        QString diagResult;      // diagresult
        QString grid;            // grid (уникальный ID)
    };

    // Тип эндоскопа (реф. KScopeClass::E_CLASS) → имя файла словаря.
    enum ScopeClass { Gastroscopy, Colonoscopy, Duodenoscope,
                      Bronchoscope, Choledochoscopy, Noselarynxscope };
    // Реф. GetFileNMameByEndoscopeType.
    static QString GetFileNameByEndoscopeType(ScopeClass cls);

    // Корень thesaurus (…/patient/thesaurus). По умолчанию — из KSystem.
    explicit KThesaurusOpt(const QString &lang = "ch",
                           const QString &thesaurusRoot = QString());

    bool Load(ScopeClass cls);              // загрузить словарь по типу
    bool LoadFile(const QString &xmlPath);  // загрузить конкретный файл

    const QVector<Record> &Records() const { return records_; }
    QStringList Groups() const;                         // уникальные diseagroup
    QVector<Record> RecordsByGroup(const QString &group) const;
    bool RecordByGrid(const QString &grid, Record &out) const;
    bool RecordByDisease(const QString &diseaName, Record &out) const;

    // Реф. AddDiseaseContent / DelDiseaseContentByGrid (правка словаря).
    QString AddDiseaseContent(const QString &group, const QString &diseaName,
                              const QString &examFinding, const QString &diagResult);
    bool DelDiseaseContentByGrid(const QString &grid);
    bool Save() const;                      // записать XML обратно

private:
    QString filePathFor(ScopeClass cls) const;
    QString lang_;
    QString root_;
    QString loadedFile_;
    QVector<Record> records_;
};
