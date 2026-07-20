#pragma once

#include <cstdint>
#include <string>

// Сторожевой процесс стойки (реф. ОТДЕЛЬНЫЙ бинарник update/root/X2000Monitor,
// 28 КБ, 31 функция, C, GCC 8.2.0, aarch64-xilinx-linux; внутреннее имя проекта
// NaChaMonitor, исходник x2000monitor.c).
//
// Следит за тремя процессами: X2000 (главный UI/мейнконтрол), X2000Video
// (видеотракт), X2000Simulator (фолбэк). Heartbeat идёт через sigqueue с
// realtime-сигналами; падение ловится по SIGCHLD + скану /proc.
//
// ЧТО ЗДЕСЬ РЕАЛИЗОВАНО: чистое ядро — приём heartbeat, таймауты, машина
// состояний, счётчики отказов, формат лога. Всё проверяемо на Mac.
// ЧТО НЕ РЕАЛИЗОВАНО (и не может быть): fork/execlp/sigqueue/kill/shutdown —
// реф. restartAPP/initAppProc/run_poweroff/mySystem/sendAppSignal/
// InstallHeartBeatSig/RecExceptionAct/find_pid_by_name. Вместо выполнения
// действий ядро ВОЗВРАЩАЕТ намерение (enum MonitorAction) — это НАШЕ решение,
// в реф. функции дёргаются напрямую; так поведение становится тестируемым.
//
// В реф. состояние — 13 файловых глобалов; у нас собрано в структуру (наше
// решение, имена полей 1:1 с реф. глобалами).
namespace x2000monitor {

// Реф. номера сигналов — СЫРЫЕ (при glibc SIGRTMIN=34 на aarch64 это
// SIGRTMIN+9 / +10 / +12; в коде именно 43/44/46).
enum Signal {
    SIG_MAINCTRL_HEARTBEAT = 43,   // heartbeat от X2000
    SIG_VIDEOMAIN_HEARTBEAT = 44,  // heartbeat от X2000Video
    SIG_CONTROL = 46,              // управляющий канал (poweroff/update/kill)
    SIG_CHLD = 17,
    SIG_SEGV = 11,
};

// Пороговые константы — все сверены дизасмом (не из отчёта):
//   таймаут heartbeat  `mov x2, #0x1388` в mainCtrlMonitor @0x28d8
//   почасовой сброс    `#0xee80 | 0x36<<16` = 0x36EE80 в runStateMachine @0x29c8
//   порог отказов      `cmp w0, #0xb` в main @0x1954/0x1960
//   период опроса      usleep(30000) в main
enum : int64_t {
    HEARTBEAT_TIMEOUT_MS = 5000,      // строго: реакция при now - last > 5000
    FAIL_RESET_PERIOD_MS = 3600000,   // 1 час
};
enum : int {
    FAIL_COUNT_LIMIT = 11,   // сдаться при счётчике > 11 (т.е. с 12-го отказа)
    POLL_INTERVAL_MS = 30,
};

// Намерение ядра (НАШЕ; в реф. — прямые вызовы).
enum MonitorAction {
    ACT_NONE = 0,
    ACT_POWEROFF,               // run_poweroff: SIGTERM всем + shutdown -h now
    ACT_RESTORE_EXIT_APP,       // restoreExitApp: разбор упавших по SIGCHLD
    ACT_RESTORE_CONSOLE_HARD,   // restoreConsole.part.0(0): SIGKILL + перезапуск
    ACT_RESTORE_CONSOLE_GRACE,  // restoreConsole.part.0(1): SIGUSR1 → SIGKILL
    ACT_RESTORE_VIDEO_HARD,     // restoreVideoMain.part.1(0)
    ACT_RESTORE_VIDEO_GRACE,    // restoreVideoMain.part.1(1)
    ACT_TIME_FAILED,            // get_time дал ошибку → лог и выход из шага
};

// Состояние (реф. 13 глобалов, адреса .data/.bss в комментариях).
struct MonitorState {
    int     g_bOpenKill = 1;            // 0x14150 — В .data ИНИЦИАЛИЗИРОВАН ЕДИНИЦЕЙ
    int     g_bAppExit = 0;             // 0x14160
    int     g_bisMainCtrlInit = 0;      // 0x14164
    int64_t g_iConsoleTime = 0;         // 0x14168
    int     g_bisUpdateSysTime = 0;     // 0x14170
    int64_t g_iVideoMainTime = 0;       // 0x14178
    int     g_bisVideoMainInit = 0;     // 0x14180
    int     g_bIsStopMonitor = 0;       // 0x14184 — НЕ сбрасывается в initAppProc
    int     g_bPoweroff = 0;            // 0x14188
    int     g_bisUpdate = 0;            // 0x1418c
    int64_t g_iFailRebootTime = 0;      // 0x14190
    int     g_iVideoMainFailCount = 0;  // 0x14198
    int     g_iMainCtrlFailCount = 0;   // 0x1419c
};

// Реф. get_time @0x1a78: gettimeofday, *pMs = sec*1000 + usec/1000; ret 0/-1.
int get_time(int64_t *pMs);

// Префикс строки лога (реф. инлайн в monitor_print_log @0x1b28):
// "[%02d-%02d %02d:%02d:%02d:%03d] " → [MM-DD HH:MM:SS:mmm], месяц tm_mon+1,
// миллисекунды tv_usec/1000. Выделено в функцию (наше) ради проверяемости.
std::string MonitorLogPrefix(int64_t secs, int64_t usecs);

// Путь лога (реф. monitor_print_log): getcwd() + "/data/app/logfile/Monitor.log";
// при отказе getcwd база — "/home/root".
std::string MonitorLogPath(const std::string &cwd);

// Реф. UpdateHeartBeatTime @0x1f88. sig 43 → отметка консоли (+ синхронизация
// отсчёта видео, если взведён g_bisUpdateSysTime); sig 44 → отметка видео.
// Иначе -1.
int UpdateHeartBeatTime(MonitorState &st, int sig, int64_t nowMs);

// Реф. RecHeartBeatAct @0x20d8 — диспетчер по (sig, si_value.sival_int).
void RecHeartBeatAct(MonitorState &st, int sig, int val, int64_t nowMs);

// Реф. mainCtrlMonitor @0x28d8 / videoMonitor @0x2950 — таймаут heartbeat.
// ⚠️ videoMonitor в РЕФ. НЕ ВЫЗЫВАЕТСЯ НИ РАЗУ (проверено: ноль call site;
// вызывается только mainCtrlMonitor). Реализован для полноты — см. .cpp.
MonitorAction mainCtrlMonitor(MonitorState &st, int64_t nowMs);
MonitorAction videoMonitor(MonitorState &st, int64_t nowMs);

// Реф. runStateMachine @0x29c8 — приоритетная цепочка проверок, шаг раз в 30 мс.
MonitorAction runStateMachine(MonitorState &st, int64_t nowMs);

// Реф. проверка в main @0x1954: если g_bOpenKill и любой счётчик > 11 — сдаться.
// Обратно включается ТОЛЬКО внешним sigqueue(46, 5); почасовой сброс счётчиков
// сам по себе g_bOpenKill не восстанавливает.
void CheckFailLimit(MonitorState &st);

} // namespace x2000monitor
