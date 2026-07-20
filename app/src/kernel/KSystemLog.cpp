#include "kernel/KSystemLog.h"

#include <QString>

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <sys/time.h>

namespace {

std::mutex g_mutex;
bool g_capture = false;
std::vector<std::string> g_lines;
int g_euLogPriority = 0;   // реф.: глобальная переменная, дефолт 0

// Реф. GetCurTime(char*, int): формат БЕЗ года.
void getCurTime(char *buf, size_t len)
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    struct tm tmv;
    localtime_r(&tv.tv_sec, &tmv);
    snprintf(buf, len, "[%02d-%02d %02d:%02d:%02d:%03d]",
             tmv.tm_mon + 1, tmv.tm_mday, tmv.tm_hour, tmv.tm_min, tmv.tm_sec,
             int(tv.tv_usec / 1000));
}

void emitLine(const char *tag, const char *fmt, va_list ap)
{
    char body[1024];
    vsnprintf(body, sizeof(body), fmt, ap);

    // Реф.: '\n' добавляется, только если формат им не оканчивается.
    std::string text(body);
    if (text.empty() || text.back() != '\n')
        text += '\n';

    std::lock_guard<std::mutex> lk(g_mutex);
    if (g_capture) {
        // Для self-test отметка времени опускается (она недетерминирована).
        std::string line = std::string(tag) + text;
        if (!line.empty() && line.back() == '\n')
            line.pop_back();
        g_lines.push_back(line);
        return;
    }
    char stamp[32];
    getCurTime(stamp, sizeof(stamp));
    fprintf(stderr, "%s%s%s", stamp, tag, text.c_str());
}

} // namespace

void LogPrintf(const char *tag, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    emitLine(tag, fmt, ap);
    va_end(ap);
}

void LogPrintfEx(bool bEnable, const char *tag, const char *fmt, ...)
{
    // Реф.: печатать iff (g_euLogPriority != 0 || bEnable).
    if (g_euLogPriority == 0 && !bEnable)
        return;
    va_list ap;
    va_start(ap, fmt);
    emitLine(tag, fmt, ap);
    va_end(ap);
}

void LogPrintfx(const char *tag, const char *file, int line, const char *func,
                const char *fmt, ...)
{
    // Реф. префикс: "%s%s%s:%d:%s :" (timeStr, tag, basename(file), line, func).
    std::string base(file ? file : "");
    const size_t slash = base.rfind('/');
    if (slash != std::string::npos)
        base = base.substr(slash + 1);
    char head[256];
    snprintf(head, sizeof(head), "%s%s:%d:%s :", tag, base.c_str(), line,
             func ? func : "");
    va_list ap;
    va_start(ap, fmt);
    emitLine(head, fmt, ap);
    va_end(ap);
}

void QStringLogPrintf(const char *tag, const QString &msg)
{
    LogPrintf(tag, "%s\n", msg.toLocal8Bit().constData());
}

void QStringLogPrintfEx(bool bEnable, const char *tag, const QString &msg)
{
    LogPrintfEx(bEnable, tag, "%s\n", msg.toLocal8Bit().constData());
}

void SetLogPriority(int p) { g_euLogPriority = p; }
int  GetLogPriority() { return g_euLogPriority; }

namespace KSystemLog {

void EnableCapture(bool on)
{
    std::lock_guard<std::mutex> lk(g_mutex);
    g_capture = on;
}

void ClearCapture()
{
    std::lock_guard<std::mutex> lk(g_mutex);
    g_lines.clear();
}

std::vector<std::string> Captured()
{
    std::lock_guard<std::mutex> lk(g_mutex);
    return g_lines;
}

std::string LogFileName(int year, int month)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "APPlog%04d-%02d.txt", year, month);
    return buf;
}

} // namespace KSystemLog
