#pragma once

#include <QString>
#include <QStringList>

// Слой доступа к ini машинного контроля (реф. KControlINI, X-2600). НЕ UI, НЕ крипто
// (DES живёт в компаньоне KControlProc), stateless (полей нет; в реф. методы инстансные,
// но this не используют — у нас static). Персист — QSettings(IniFormat), НЕ наш KConfig:
// формат bool у QSettings (true/false) и сериализация QStringList несовместимы с KConfig
// ("True"/"False"), поэтому для байт-в-байт совместимости используется именно QSettings.
//
// Корень: KSystem::ProtectedPath() + "kmachinecontrol/". Реально читается/пишется
// ТОЛЬКО control.ini (плоские ключи → секция [General]); path-хелперы для plain.ini/
// matchprolist.ini/licensehistory.ini обслуживает KControlProc.

// Блок временного контроля (реф. _MC_Time).
struct _MC_Time {
    bool    controlTime = false;   // Control_time
    QString deadline;              // Deadline
    int     remainDays = 0;        // RemainDays
};

// Блок контроля совместимости эндоскопов (реф. _MC_Endo).
struct _MC_Endo {
    bool        controlEndo = false;   // Control_endo
    QStringList endos;                 // Endos
};

class KControlINI
{
public:
    // Пути (реф.: MachineControlPath создаёт каталог, если его нет).
    static QString MachineControlPath(const QString &name);
    static QString ControlINIPath();        // control.ini
    static QString PlainINIpath();          // plain.ini      (хелпер; сам класс не читает)
    static QString MatchProListIni();       // matchprolist.ini (хелпер)
    static QString HistoryLicenseRecord();  // licensehistory.ini (хелпер)

    // Блочное чтение/запись control.ini.
    static void ReadMcTime(_MC_Time &out);
    static void WriteMcTime(const _MC_Time &in);
    static void ReadMcEndo(_MC_Endo &out);
    static void WriteMcEndo(const _MC_Endo &in);

    // Поэлементный доступ (те же ключи control.ini).
    static QString GetDeadline();               // дефолт "2099-01-01"
    static void    SetDeadline(const QString &v);
    static int     GetRemainDays();             // дефолт 0
    static void    SetRemainDays(int v);
    static QStringList GetMatchEndos();         // ключ Endos (в control.ini, не matchprolist!)
    static void    SetMatchEndos(const QStringList &v);

    static bool IsStartTimeControl();           // Control_time, дефолт false
    static void StartTimeControl(bool v);
    static bool IsStartEndoControl();           // Control_endo, дефолт false
    static void StartEndoControl(bool v);

    // UpdateMatchEndos(_MC_InputInfo*) НЕ реализован: структура _MC_InputInfo и правило
    // add/remove из дизасма не восстановлены (известны лишь int@0/int@8). Не выдумываем —
    // строительные блоки Get/SetMatchEndos доступны.
};
