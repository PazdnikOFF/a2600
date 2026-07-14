#include "db/KExamListConfigHandler.h"
#include "sys/KSystem.h"

#include <QSettings>
#include <QDir>

KExamListConfigHandler &KExamListConfigHandler::GetInstance()
{
    static KExamListConfigHandler inst;
    return inst;
}

QString KExamListConfigHandler::cfgFile() const
{
    if (!cfgFile_.isEmpty())
        return cfgFile_;
    return QDir(KSystem::SystemPath())
        .absoluteFilePath("presetdata/userpreset/examlistconfig.ini");
}

bool KExamListConfigHandler::get(const QString &key, bool def) const
{
    QSettings ini(cfgFile(), QSettings::IniFormat);
    return ini.value("ExamList/" + key, def).toBool();
}

int KExamListConfigHandler::get(const QString &key, int def) const
{
    QSettings ini(cfgFile(), QSettings::IniFormat);
    return ini.value("ExamList/" + key, def).toInt();
}

void KExamListConfigHandler::set(const QString &key, bool v)
{
    QSettings ini(cfgFile(), QSettings::IniFormat);
    ini.setValue("ExamList/" + key, v);
}

void KExamListConfigHandler::set(const QString &key, int v)
{
    QSettings ini(cfgFile(), QSettings::IniFormat);
    ini.setValue("ExamList/" + key, v);
}

QString KExamListConfigHandler::GetExportPath() const
{
    QSettings ini(cfgFile(), QSettings::IniFormat);
    return ini.value("ExamList/ExportPath").toString();
}

void KExamListConfigHandler::SetExportPath(const QString &p)
{
    QSettings ini(cfgFile(), QSettings::IniFormat);
    ini.setValue("ExamList/ExportPath", p);
}

void KExamListConfigHandler::SaveConfig()
{
    QSettings ini(cfgFile(), QSettings::IniFormat);
    ini.sync();
}
