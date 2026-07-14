#pragma once

#include <QString>
#include <QVector>

// Конфигурация статистики/событий (реф. statistic.ini + KStatisticInfo, X-2600).
// system/presetdata/syspreset/statistic.ini задаёт спеку событий устройства для
// извлечения из системного лога. Ключи с префиксом:
//   time_<name> — событие с меткой времени (power on/off, endo insert/remove,
//                 examine_start, video_start/stop);
//   dcnt_<name> — счётчик (frame lost);
//   info_<name> — извлечение значения (температура).
// Значение = строка-паттерн, по которой событие ищется в логе.
// Секции: [Common] (жизненный цикл), [Video], [Special] (температура).
class KStatisticConfig
{
public:
    enum Kind { Time, Count, Info, Other };
    struct Event {
        QString section;   // Common/Video/Special
        QString key;       // полный ключ (time_power_on)
        Kind    kind = Other;
        QString name;      // без префикса (power_on)
        QString pattern;   // строка-паттерн в логе
    };

    bool Load(const QString &iniPath = QString());   // по умолчанию — из KSystem

    const QVector<Event> &Events() const { return events_; }
    QVector<Event> EventsOfKind(Kind k) const;
    QString PatternFor(const QString &key) const;    // паттерн по полному ключу

    static Kind KindOf(const QString &key);          // префикс → Kind

private:
    QString cfgFile(const QString &given) const;
    QVector<Event> events_;
};
