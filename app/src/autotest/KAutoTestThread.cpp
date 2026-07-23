#include "autotest/KAutoTestThread.h"

#include "sys/KSystem.h"
#include "sys/KSystemStatus.h"

#include <QDate>
#include <QDir>
#include <QProcess>

namespace {
KAutoTestThread::IKeySink *g_sink = nullptr;

// Реф. модификаторы приходят в СТАРШИХ битах кода клавиши; дизасм сдвигает значение
// на 4 вправо и работает с битами 25..27 (см. `lsr w19, w1, #4`).
enum { MOD_SHIFT = 1 << 25, MOD_CTRL = 1 << 26, MOD_ALT = 1 << 27 };

// Раннер проверки лога. По умолчанию — как в реф.: запуск python-скрипта прошивки.
std::function<bool()> g_logCheckRunner;
}

void KAutoTestThread::SetKeySink(IKeySink *sink) { g_sink = sink; }
void KAutoTestThread::SetLogCheckRunner(std::function<bool()> f) { g_logCheckRunner = f; }

KAutoTestThread *GetKAutoTestThread()
{
    // Реф.: статический указатель, ленивое new(0x128), delete НИКОГДА.
    static KAutoTestThread *inst = nullptr;
    if (!inst)
        inst = new KAutoTestThread(nullptr);
    return inst;
}

KAutoTestThread::KAutoTestThread(QObject *parent)
    : QThread(parent)
{
    ResetFileExecCount();
    // Реф. дальше зовёт StartProcMsgManager(0x85000000) и (0x86000000) — подписка на
    // два канала IPC (device): именно оттуда приходят RecvMsg.
}

void KAutoTestThread::ResetFileExecCount()
{
    // ⚠️ Реф. пишет ОДНОЙ невыравненной 8-байтовой командой: +0x14 = 1, +0x18 = 0.
    m_nFileExecCount = 1;
    m_nExecIndex = 0;
}

void KAutoTestThread::SetLogCheckOpen(bool open) { m_bLogCheckOpen = open; }

void KAutoTestThread::start(Priority priority)
{
    if (isRunning())
        return;                     // реф.: повторный старт — молча ничего
    m_bStop = false;
    // Реф. здесь Setup_uinput_device() — device; у нас сток задаётся снаружи.
    QThread::start(priority);
}

void KAutoTestThread::stop()
{
    m_bStop = true;                 // реф.: только флаг, без wait()
    // Реф. Remove_uinput_device() — device.
}

QString KAutoTestThread::GetLogPath() const
{
    // Реф.: strncpy(LogPath) + snprintf("APPlog%04d-%02d.txt", год, месяц) + strncat.
    // Год и месяц — ТЕКУЩИЕ (localtime); при отказе localtime_r реф. подставляет 1900-01.
    const QDate now = QDate::currentDate();
    const QString name = QString::asprintf("APPlog%04d-%02d.txt", now.year(), now.month());
    // (в реф. LogPath оканчивается на "/", у нас слэш добавляет QDir)
    return QDir(KSystem::LogPath()).absoluteFilePath(name);
}

bool KAutoTestThread::IsAutoTestStart()
{
    const int st = KSystemStatus::GetInstance().AutoTestStatus();
    // Реф. `tst w0, #0xfffffffd; cset ne` — ложь ТОЛЬКО при 0 и при 2.
    return st != 0 && st != 2;
}

void KAutoTestThread::KeyboardSimulation(int param)
{
    if (!g_sink)
        return;
    // Реф.: код клавиши — младшие 28 бит, дальше QtKey2InputKey (таблица Qt→Linux, device).
    const int key = param & 0x0FFFFFFF;
    const int mods = (param >> 4) & (MOD_SHIFT | MOD_CTRL | MOD_ALT);

    // ⚠️ Порядок проверок в реф. именно такой (от самой полной комбинации к пустой).
    if (mods == (MOD_SHIFT | MOD_CTRL | MOD_ALT))
        g_sink->CtrlAltShiftKey(key);
    else if ((mods & (MOD_CTRL | MOD_ALT)) == (MOD_CTRL | MOD_ALT))
        g_sink->CtrlAltKey(key);
    else if ((mods & (MOD_SHIFT | MOD_CTRL)) == (MOD_SHIFT | MOD_CTRL))
        g_sink->CtrlShiftKey(key);
    else if ((mods & (MOD_SHIFT | MOD_ALT)) == (MOD_SHIFT | MOD_ALT))
        g_sink->AltShiftKey(key);
    else if (mods & MOD_CTRL)
        g_sink->CtrlKey(key);
    else if (mods & MOD_ALT)
        g_sink->AltKey(key);
    else if (mods & MOD_SHIFT)
        g_sink->ShiftKey(key);
    else
        g_sink->Key(key);
}

void KAutoTestThread::PanelKeySimulation(int key, int value)
{
    if (!g_sink)
        return;
    // Реф. раскладка сообщения 0x41A: +8 — key (u16), +10 — (value >> 16) & 0xff,
    // +11 — value & 0xff. Затем KSrvBaseThread::PostMsg(1, &msg).
    g_sink->PanelKey(key & 0xFFFF, (value >> 16) & 0xFF, value & 0xFF);
}

void KAutoTestThread::EnqueueKey(int key, int event)
{
    m_keyQueue.enqueue(qMakePair(key, event));
}

void KAutoTestThread::ProcessPendingKeys()
{
    while (!m_keyQueue.isEmpty()) {
        const QPair<int, int> item = m_keyQueue.dequeue();
        // Реф.: пара {0,0} пропускается (`orr w0,w23,w24; cbz`).
        if (item.first == 0 && item.second == 0)
            continue;
        // ⚠️ Развилка реф.: event == 0 → клавиатура, иначе — панель.
        if (item.second == 0)
            KeyboardSimulation(item.first);
        else
            PanelKeySimulation(item.first, item.second);
    }
}

void KAutoTestThread::run()
{
    while (!m_bStop) {
        QThread::usleep(100000);   // реф. 0x186A0 мкс = 100 мс
        ProcessPendingKeys();
    }
}

bool KAutoTestThread::AutotestLogCheck()
{
    if (g_logCheckRunner)
        return g_logCheckRunner();

    // Реф.: QProcess.start("python", [logcheck.py, <лог>, rulesfile]),
    // waitForStarted(30000) / waitForFinished(30000), возврат = (exitCode() == 0).
    // ⚠️ Скрипт прошивки — Python 2 (синтаксис `print '...'`), поэтому на хосте с
    // одним python3 он падает; это device-поведение, здесь оставлено как есть.
    const QString base = QDir(KSystem::SystemPath()).absoluteFilePath("autotest/logcheck");
    const QString script = QDir(base).absoluteFilePath("logcheck.py");
    const QString rules  = QDir(base).absoluteFilePath("rulesfile");

    QProcess proc;
    proc.start("python", QStringList() << script << GetLogPath() << rules);
    if (!proc.waitForStarted(30000))
        return false;              // реф.: «Process start failed» → возврат 0
    if (!proc.waitForFinished(30000))
        return false;
    proc.readAllStandardOutput();  // реф. читает вывод (для лога), решает по exitCode
    return proc.exitCode() == 0;
}

void KAutoTestThread::LogCheckRecord(int stage)
{
    if (!m_bLogCheckOpen)          // реф.: поле +0x124 снято → выход сразу
        return;

    if (stage == 1) {
        // Реф.: ShowMsg("") (пустая строка) + маркер «---autotest log start---» в лог.
        emit ShowMsg(QString());
        // (маркер уходит в APP-лог через LogPrintf — device-логгер, здесь опущен)
    } else if (stage == 2) {
        // Маркер «---autotest log stop---», затем — если автотест НЕ прерван (статус 2) —
        // прогон проверки лога; при провале ShowMsg + аварийная остановка автотеста.
        if (KSystemStatus::GetInstance().AutoTestStatus() == 2)
            return;
        if (!AutotestLogCheck()) {
            emit ShowMsg(QStringLiteral("autotest log check failed"));
            SetAutoTestOpen(TEST_ABORT, 0x800);
        }
    }
    // Прочие stage реф. игнорирует.
}

void SetAutoTestOpen(int status, int /*type*/)
{
    // Реф. @0x6fc8a0: сначала фиксируем статус в центральном состоянии…
    KSystemStatus::GetInstance().SetAutoTestStatus(status);
    // …затем поднимаем/останавливаем поток по статусу (1 = старт, 2 = стоп).
    if (status == TEST_START)
        GetKAutoTestThread()->start();
    else if (status == TEST_ABORT)
        GetKAutoTestThread()->stop();
    // Реф. далее шлёт IPC KProcMsgManager::SendMsg(0x85000000, …) — device, опущено.
}
