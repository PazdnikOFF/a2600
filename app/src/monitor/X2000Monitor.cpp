#include "X2000Monitor.h"

#include <cstdio>
#include <ctime>

#include <sys/time.h>

namespace x2000monitor {

int get_time(int64_t *pMs)
{
    // Реф. @0x1a78: gettimeofday(&tv, 0); *p = tv.sec*1000 + tv.usec/1000.
    if (!pMs)
        return -1;
    struct timeval tv;
    if (gettimeofday(&tv, nullptr) != 0)
        return -1;
    *pMs = static_cast<int64_t>(tv.tv_sec) * 1000 + tv.tv_usec / 1000;
    return 0;
}

std::string MonitorLogPrefix(int64_t secs, int64_t usecs)
{
    // Реф. формат "[%02d-%02d %02d:%02d:%02d:%03d] " — БЕЗ года.
    // Месяц печатается как tm_mon + 1, миллисекунды — tv_usec / 1000.
    const time_t t = static_cast<time_t>(secs);
    struct tm lt;
    localtime_r(&t, &lt);

    char buf[40];
    std::snprintf(buf, sizeof(buf), "[%02d-%02d %02d:%02d:%02d:%03d] ",
                  lt.tm_mon + 1, lt.tm_mday,
                  lt.tm_hour, lt.tm_min, lt.tm_sec,
                  static_cast<int>(usecs / 1000));
    return std::string(buf);
}

std::string MonitorLogPath(const std::string &cwd)
{
    // Реф.: база = getcwd(), при ошибке — литерал "/home/root";
    // далее "%s/%s/Monitor.log" с "data/app/logfile".
    const std::string base = cwd.empty() ? std::string("/home/root") : cwd;
    return base + "/data/app/logfile/Monitor.log";
}

int UpdateHeartBeatTime(MonitorState &st, int sig, int64_t nowMs)
{
    // Реф. @0x1f88.
    if (sig == SIG_MAINCTRL_HEARTBEAT) {
        st.g_bisMainCtrlInit = 1;
        st.g_iConsoleTime = nowMs;
        // Запрос синхронизации: отсчёт видео подтягивается к консоли, флаг гасится.
        if (st.g_bisUpdateSysTime) {
            st.g_bisUpdateSysTime = 0;
            st.g_iVideoMainTime = nowMs;
        }
        return 0;
    }
    if (sig == SIG_VIDEOMAIN_HEARTBEAT) {
        st.g_iVideoMainTime = nowMs;
        st.g_bisVideoMainInit = 1;
        return 0;
    }
    return -1;   // реф. логирует "Not support sig to update heart beat."
}

void RecHeartBeatAct(MonitorState &st, int sig, int val, int64_t nowMs)
{
    // Реф. @0x20d8 — диспетчер по номеру сигнала и si_value.sival_int.
    switch (sig) {
    case SIG_MAINCTRL_HEARTBEAT:
        // Полезная нагрузка модифицирует флаги, но heartbeat отмечается ВСЕГДА.
        if (val == 1)
            st.g_bisUpdateSysTime = 1;
        else if (val == 2)
            st.g_bIsStopMonitor = 1;
        else if (val == 3)
            st.g_bIsStopMonitor = 0;
        UpdateHeartBeatTime(st, SIG_MAINCTRL_HEARTBEAT, nowMs);
        break;

    case SIG_VIDEOMAIN_HEARTBEAT:
        // Значение игнорируется.
        UpdateHeartBeatTime(st, SIG_VIDEOMAIN_HEARTBEAT, nowMs);
        break;

    case SIG_CONTROL:
        if (val == 0)      st.g_bPoweroff = 1;
        else if (val == 1) st.g_bisUpdate = 1;
        else if (val == 2) st.g_bisUpdate = 0;
        else if (val == 5) st.g_bOpenKill = 1;
        else if (val == 6) st.g_bOpenKill = 0;
        break;

    case SIG_CHLD:
        // Реф.: waitpid(-1,…,WNOHANG) в цикле, затем g_bAppExit = 1.
        st.g_bAppExit = 1;
        break;

    default:
        // Реф. логирует "Not support sig:%d".
        break;
    }
}

MonitorAction mainCtrlMonitor(MonitorState &st, int64_t nowMs)
{
    // Реф. @0x28d8: порог 5000 мс СТРОГИЙ (b.gt, т.е. реакция при > 5000).
    // Различаются два случая: heartbeat ещё ни разу не приходил (init timeout)
    // и приходил, но протух. Действие в обоих одно — graceful-перезапуск.
    if (nowMs - st.g_iConsoleTime <= HEARTBEAT_TIMEOUT_MS)
        return ACT_NONE;

    ++st.g_iMainCtrlFailCount;
    // Реф. лог: "X2000 init timeout." при !g_bisMainCtrlInit, иначе "X2000 timeout."
    st.g_iConsoleTime = nowMs;
    if (!st.g_bOpenKill)
        return ACT_NONE;
    st.g_bisMainCtrlInit = 0;
    return ACT_RESTORE_CONSOLE_GRACE;
}

MonitorAction videoMonitor(MonitorState &st, int64_t nowMs)
{
    // Реф. @0x2950. ⚠️ МЁРТВЫЙ КОД: в бинарнике НИ ОДНОГО call site (проверено
    // дизасмом — из runStateMachine вызывается только mainCtrlMonitor @0x28d8).
    // Следствие для модели прибора: таймаут heartbeat видеотракта фактически
    // НЕ отслеживается, падение X2000Video ловится только по SIGCHLD.
    // Реализовано для полноты; runStateMachine его НЕ зовёт — как в реф.
    if (nowMs - st.g_iVideoMainTime <= HEARTBEAT_TIMEOUT_MS)
        return ACT_NONE;

    ++st.g_iVideoMainFailCount;
    st.g_iVideoMainTime = nowMs;
    if (!st.g_bOpenKill)
        return ACT_NONE;
    st.g_bisVideoMainInit = 0;
    return ACT_RESTORE_VIDEO_GRACE;
}

MonitorAction runStateMachine(MonitorState &st, int64_t nowMs)
{
    // Реф. @0x29c8 — приоритетная цепочка, шаг каждые 30 мс.
    // 1. Выключение — раньше всего и не возвращается.
    if (st.g_bPoweroff)
        return ACT_POWEROFF;

    // 2. Мониторинг заморожен: пауза по команде либо идёт обновление ПО.
    if (st.g_bIsStopMonitor || st.g_bisUpdate)
        return ACT_NONE;

    // 3. Кто-то упал (взведено из SIGCHLD).
    if (st.g_bAppExit)
        return ACT_RESTORE_EXIT_APP;

    // 4. Сдались (12 отказов за час) либо внешне запрещено.
    if (!st.g_bOpenKill)
        return ACT_NONE;

    // 5. Таймаут heartbeat — ТОЛЬКО консоль (videoMonitor не вызывается, см. выше).
    const MonitorAction act = mainCtrlMonitor(st, nowMs);

    // 6. Почасовой сброс счётчиков отказов.
    if (nowMs - st.g_iFailRebootTime > FAIL_RESET_PERIOD_MS) {
        st.g_iFailRebootTime = nowMs;
        st.g_iVideoMainFailCount = 0;
        st.g_iMainCtrlFailCount = 0;
    }
    return act;
}

void CheckFailLimit(MonitorState &st)
{
    // Реф. main @0x1954: cmp w0, #0xb — сдаёмся, когда счётчик СТРОГО больше 11.
    if (st.g_bOpenKill
        && (st.g_iMainCtrlFailCount > FAIL_COUNT_LIMIT
            || st.g_iVideoMainFailCount > FAIL_COUNT_LIMIT)) {
        st.g_bOpenKill = 0;
    }
}

} // namespace x2000monitor
