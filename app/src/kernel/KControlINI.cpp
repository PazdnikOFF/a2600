#include "kernel/KControlINI.h"
#include "sys/KSystem.h"

#include <QDir>
#include <QSettings>

namespace {
// Ключи control.ini — плоские (реф. без beginGroup → секция [General]).
const char *K_CONTROL_TIME = "Control_time";
const char *K_DEADLINE     = "Deadline";
const char *K_REMAIN_DAYS  = "RemainDays";
const char *K_CONTROL_ENDO = "Control_endo";
const char *K_ENDOS        = "Endos";
const char *DEFAULT_DEADLINE = "2099-01-01";
} // namespace

// QSettings некопируем — работаем через макрос локального объекта над control.ini.
#define MC_INI QSettings s(KControlINI::ControlINIPath(), QSettings::IniFormat)

QString KControlINI::MachineControlPath(const QString &name)
{
    const QString dir = QDir(KSystem::ProtectedPath()).absoluteFilePath("kmachinecontrol");
    if (!QDir(dir).exists())
        QDir().mkpath(dir);   // реф. создаёт каталог при отсутствии
    return QDir(dir).absoluteFilePath(name);
}

QString KControlINI::ControlINIPath()       { return MachineControlPath("control.ini"); }
QString KControlINI::PlainINIpath()         { return MachineControlPath("plain.ini"); }
QString KControlINI::MatchProListIni()      { return MachineControlPath("matchprolist.ini"); }
QString KControlINI::HistoryLicenseRecord() { return MachineControlPath("licensehistory.ini"); }

void KControlINI::ReadMcTime(_MC_Time &out)
{
    MC_INI;
    out.controlTime = s.value(K_CONTROL_TIME, false).toBool();
    out.deadline    = s.value(K_DEADLINE, DEFAULT_DEADLINE).toString();
    out.remainDays  = s.value(K_REMAIN_DAYS, 0).toInt();
}

void KControlINI::WriteMcTime(const _MC_Time &in)
{
    MC_INI;
    s.setValue(K_CONTROL_TIME, in.controlTime);
    s.setValue(K_DEADLINE, in.deadline);
    s.setValue(K_REMAIN_DAYS, in.remainDays);
}

void KControlINI::ReadMcEndo(_MC_Endo &out)
{
    MC_INI;
    out.controlEndo = s.value(K_CONTROL_ENDO, false).toBool();
    out.endos       = s.value(K_ENDOS).toStringList();
}

void KControlINI::WriteMcEndo(const _MC_Endo &in)
{
    MC_INI;
    s.setValue(K_CONTROL_ENDO, in.controlEndo);
    s.setValue(K_ENDOS, in.endos);
}

QString KControlINI::GetDeadline()
{
    MC_INI;
    return s.value(K_DEADLINE, DEFAULT_DEADLINE).toString();
}
void KControlINI::SetDeadline(const QString &v)
{
    MC_INI;
    s.setValue(K_DEADLINE, v);
}

int KControlINI::GetRemainDays()
{
    MC_INI;
    return s.value(K_REMAIN_DAYS, 0).toInt();
}
void KControlINI::SetRemainDays(int v)
{
    MC_INI;
    s.setValue(K_REMAIN_DAYS, v);
}

QStringList KControlINI::GetMatchEndos()
{
    MC_INI;
    return s.value(K_ENDOS).toStringList();
}
void KControlINI::SetMatchEndos(const QStringList &v)
{
    MC_INI;
    s.setValue(K_ENDOS, v);
}

bool KControlINI::IsStartTimeControl()
{
    MC_INI;
    return s.value(K_CONTROL_TIME, false).toBool();
}
void KControlINI::StartTimeControl(bool v)
{
    MC_INI;
    s.setValue(K_CONTROL_TIME, v);
}

bool KControlINI::IsStartEndoControl()
{
    MC_INI;
    return s.value(K_CONTROL_ENDO, false).toBool();
}
void KControlINI::StartEndoControl(bool v)
{
    MC_INI;
    s.setValue(K_CONTROL_ENDO, v);
}
