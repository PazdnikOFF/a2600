#include "sys/KPatientTimeOperation.h"

#include <QTime>

QDate KPatientTimeOperation::parseYYYYMMDD(const QString &yyyymmdd)
{
    // Внутренний формат — 8 цифр YYYYMMDD.
    if (yyyymmdd.size() != 8)
        return QDate();
    return QDate::fromString(yyyymmdd, "yyyyMMdd");
}

QString KPatientTimeOperation::ConvertYYYYMMDDToDbDate(const QString &yyyymmdd)
{
    const QDate d = parseYYYYMMDD(yyyymmdd);
    return d.isValid() ? d.toString("yyyy-MM-dd") : QString();
}

QString KPatientTimeOperation::ConvertYYYYMMDDToDicomDate(const QString &yyyymmdd)
{
    // DICOM DA — те же 8 цифр; возвращаем только при валидной дате.
    const QDate d = parseYYYYMMDD(yyyymmdd);
    return d.isValid() ? d.toString("yyyyMMdd") : QString();
}

QString KPatientTimeOperation::ConvertYYYYMMDDToDate(const QString &yyyymmdd,
                                                     const QString &displayFormat)
{
    const QDate d = parseYYYYMMDD(yyyymmdd);
    return d.isValid() ? d.toString(displayFormat) : QString();
}

QString KPatientTimeOperation::ConvertHHMMSSToDbTime(const QString &hhmmss)
{
    if (hhmmss.size() != 6)
        return QString();
    const QTime t = QTime::fromString(hhmmss, "HHmmss");
    return t.isValid() ? t.toString("HH:mm:ss") : QString();
}

QString KPatientTimeOperation::ConvertHHMMSSToDicomTime(const QString &hhmmss)
{
    if (hhmmss.size() != 6)
        return QString();
    const QTime t = QTime::fromString(hhmmss, "HHmmss");
    return t.isValid() ? t.toString("HHmmss") : QString();
}

int KPatientTimeOperation::GetAgeByBirthDay(const QString &birthDay, const QDate &today)
{
    const QDate birth = QDate::fromString(birthDay, "yyyy-MM-dd");
    if (!birth.isValid())
        return -1;
    const QDate ref = today.isValid() ? today : QDate::currentDate();
    if (birth > ref)
        return -1;
    // Полных лет: разница годов минус 1, если день рождения в этом году ещё не наступил.
    int age = ref.year() - birth.year();
    if (ref.month() < birth.month() ||
        (ref.month() == birth.month() && ref.day() < birth.day()))
        --age;
    return age;
}

bool KPatientTimeOperation::IsYYYYMMDDDicomDateRange(const QString &range)
{
    // DICOM: одиночная дата "yyyyMMdd" или диапазон "yyyyMMdd-yyyyMMdd".
    if (range.contains('-')) {
        const QStringList parts = range.split('-');
        if (parts.size() != 2)
            return false;
        return parseYYYYMMDD(parts[0]).isValid() && parseYYYYMMDD(parts[1]).isValid();
    }
    return parseYYYYMMDD(range).isValid();
}
