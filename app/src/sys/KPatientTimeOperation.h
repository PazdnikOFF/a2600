#pragma once

#include <QString>
#include <QDate>

// Конвертеры дат/времени пациента (реф. KPatientTimeOperation, X-2600). Чистая
// логика форматирования между внутренним YYYYMMDD/HHMMSS, форматом БД, DICOM (DA/TM)
// и отображаемым; расчёт возраста по дате рождения.
//
// Форматы (1:1 с бинарником): БД-дата "yyyy-MM-dd", БД-время "HH:mm:ss",
// DICOM DA "yyyyMMdd", DICOM TM "HHmmss". Отображаемая дата — по системному формату
// (DD/MM/YYYY | MM/DD/YYYY | yyyy-MM-dd), передаётся параметром.
class KPatientTimeOperation
{
public:
    // YYYYMMDD → "yyyy-MM-dd" (реф. ConvertYYYYMMDDToDbDate). "" при ошибке.
    static QString ConvertYYYYMMDDToDbDate(const QString &yyyymmdd);
    // YYYYMMDD → DICOM DA "yyyyMMdd" (валидация 8 цифр; реф. ConvertYYYYMMDDToDicomDate).
    static QString ConvertYYYYMMDDToDicomDate(const QString &yyyymmdd);
    // YYYYMMDD → отображаемая дата по формату Qt (реф. ConvertYYYYMMDDToDate).
    static QString ConvertYYYYMMDDToDate(const QString &yyyymmdd,
                                         const QString &displayFormat = "yyyy-MM-dd");

    // HHMMSS → "HH:mm:ss" (БД; реф. ConvertHHMMSSToDbTime).
    static QString ConvertHHMMSSToDbTime(const QString &hhmmss);
    // HHMMSS → DICOM TM "HHmmss" (реф. ConvertHHMMSSToDicomTime).
    static QString ConvertHHMMSSToDicomTime(const QString &hhmmss);

    // Возраст (полных лет) по дате рождения "yyyy-MM-dd" (реф. GetAgeByBirthDay).
    // today — опорная дата (по умолчанию QDate::currentDate()); инъекция для теста.
    static int GetAgeByBirthDay(const QString &birthDay, const QDate &today = QDate());

    // Валиден ли DICOM-диапазон дат "yyyyMMdd" или "yyyyMMdd-yyyyMMdd"
    // (реф. IsYYYYMMDDDicomDateRange).
    static bool IsYYYYMMDDDicomDateRange(const QString &range);

private:
    static QDate parseYYYYMMDD(const QString &yyyymmdd);
};
