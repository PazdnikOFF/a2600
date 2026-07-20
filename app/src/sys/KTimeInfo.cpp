#include "sys/KTimeInfo.h"

#include <chrono>
#include <cstdio>
#include <ctime>

long long KTimeInfo::s_fixedTime = 0;

void KTimeInfo::SetFixedTime(long long unixSeconds) { s_fixedTime = unixSeconds; }

KTimeInfo::KTimeInfo()
{
    // Реф.: все 7 полей инициализируются пустой строкой, затем Init().
    Init();
}

void KTimeInfo::Init()
{
    long long ns;
    if (s_fixedTime) {
        ns = s_fixedTime * 1000000000LL;
    } else {
        ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                 std::chrono::system_clock::now().time_since_epoch())
                 .count();
    }
    const long long secs = ns / 1000000000LL;
    const int msec = int(ns / 1000000LL - secs * 1000LL);

    const std::time_t t = std::time_t(secs);
    const std::tm *tmv = std::localtime(&t);   // реф.: именно localtime
    if (!tmv)
        return;

    char buf[16];
    snprintf(buf, sizeof(buf), "%04d", tmv->tm_year + 1900); m_strYear = buf;
    snprintf(buf, sizeof(buf), "%02d", tmv->tm_mon + 1);     m_strMonth = buf;
    snprintf(buf, sizeof(buf), "%02d", tmv->tm_mday);        m_strDay = buf;
    snprintf(buf, sizeof(buf), "%02d", tmv->tm_hour);        m_strHour = buf;
    snprintf(buf, sizeof(buf), "%02d", tmv->tm_min);         m_strMin = buf;
    snprintf(buf, sizeof(buf), "%02d", tmv->tm_sec);         m_strSec = buf;
    snprintf(buf, sizeof(buf), "%03d", msec);                m_strMsec = buf;
}

std::string KTimeInfo::GetCurrentTimeYYYY() const { return m_strYear; }

std::string KTimeInfo::GetCurrentTimeYYYYMM(std::string sep) const
{
    return m_strYear + sep + m_strMonth;
}

std::string KTimeInfo::GetCurrentTimeYYYYMMDD(std::string sep) const
{
    return m_strYear + sep + m_strMonth + sep + m_strDay;
}

std::string KTimeInfo::GetCurrentTimeHHMMSS(std::string sep) const
{
    return m_strHour + sep + m_strMin + sep + m_strSec;
}

std::string KTimeInfo::GetCurrentTimeYYYYMMDD_HHMMSS(std::string dateSep,
                                                     std::string timeSep) const
{
    // Реф.: разделитель '_' зашит в код.
    return GetCurrentTimeYYYYMMDD(dateSep) + "_" + GetCurrentTimeHHMMSS(timeSep);
}

long long GetMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
