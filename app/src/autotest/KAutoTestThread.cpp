#include "autotest/KAutoTestThread.h"

#include "sys/KSystem.h"
#include "sys/KSystemStatus.h"

#include <QDate>
#include <QDir>

namespace {
KAutoTestThread::IKeySink *g_sink = nullptr;

// Реф. модификаторы приходят в СТАРШИХ битах кода клавиши; дизасм сдвигает значение
// на 4 вправо и работает с битами 25..27 (см. `lsr w19, w1, #4`).
enum { MOD_SHIFT = 1 << 25, MOD_CTRL = 1 << 26, MOD_ALT = 1 << 27 };
}

void KAutoTestThread::SetKeySink(IKeySink *sink) { g_sink = sink; }

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
