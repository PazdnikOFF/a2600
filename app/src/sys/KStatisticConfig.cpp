#include "sys/KStatisticConfig.h"
#include "sys/KSystem.h"

#include <QSettings>
#include <QDir>

KStatisticConfig::Kind KStatisticConfig::KindOf(const QString &key)
{
    if (key.startsWith("time_")) return Time;
    if (key.startsWith("dcnt_")) return Count;
    if (key.startsWith("info_")) return Info;
    return Other;
}

QString KStatisticConfig::cfgFile(const QString &given) const
{
    if (!given.isEmpty())
        return given;
    return QDir(KSystem::SystemPath())
        .absoluteFilePath("presetdata/syspreset/statistic.ini");
}

bool KStatisticConfig::Load(const QString &iniPath)
{
    events_.clear();
    QSettings ini(cfgFile(iniPath), QSettings::IniFormat);
    for (const QString &section : ini.childGroups()) {
        ini.beginGroup(section);
        for (const QString &key : ini.childKeys()) {
            Event e;
            e.section = section;
            e.key = key;
            e.kind = KindOf(key);
            const int us = key.indexOf('_');
            e.name = (us >= 0) ? key.mid(us + 1) : key;
            // Значение может авто-разбиваться QSettings по запятой → соберём обратно.
            e.pattern = ini.value(key).toStringList().join(',');
            if (e.pattern.isEmpty())
                e.pattern = ini.value(key).toString();
            events_.append(e);
        }
        ini.endGroup();
    }
    return !events_.isEmpty();
}

QVector<KStatisticConfig::Event> KStatisticConfig::EventsOfKind(Kind k) const
{
    QVector<Event> out;
    for (const Event &e : events_)
        if (e.kind == k)
            out.append(e);
    return out;
}

QString KStatisticConfig::PatternFor(const QString &key) const
{
    for (const Event &e : events_)
        if (e.key == key)
            return e.pattern;
    return QString();
}
