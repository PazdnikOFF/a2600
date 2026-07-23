#pragma once

#include <QQueue>
#include <QString>
#include <QThread>

// Приёмная половина автотеста в X2000 (реф. KAutoTestThread @0x6fb4d8, sizeof 0x128).
// Скрипты разбирает ОТДЕЛЬНЫЙ процесс X2000Simulator (см. KAutoTestScript.h); сюда по IPC
// приходят уже готовые коды, а поток раз в 100 мс достаёт их из очереди и «нажимает».
//
// Синглтон отдаёт свободная GetKAutoTestThread() @0x6fc830 (ленивое new, delete никогда).
//
// ⚠️ ЧТО ЗДЕСЬ ЕЩЁ НЕ ПОРТИРОВАНО (не декодировано, следующая итерация):
//   • ветки RecvMsg с типом 1 и 2 (разбор клавиши/панели из _KMsgBuf и постановка в очередь);
//   • половина «проверки логов»: GetSnapScreenPngFile @0x6fb558, GetPngFileIndex @0x6fc748,
//     AutotestLogCheck @0x6fcb00, LogCheckRecord @0x6fd028.
// Из-за этого очередь наполняется методом EnqueueKey (не из реф.) — им же пользуется self-test.
class KAutoTestThread : public QThread
{
    Q_OBJECT
public:
    // Куда уходят «нажатия». В реф. это свободные функции над /dev/uinput
    // (Key/ShiftKey/CtrlKey/AltKey/CtrlAltKey/CtrlShiftKey/AltShiftKey/CtrlAltShiftKey)
    // и KSrvBaseThread::PostMsg для панели — единственный device-шов класса.
    class IKeySink
    {
    public:
        virtual ~IKeySink() {}
        virtual void Key(int key) = 0;
        virtual void ShiftKey(int key) = 0;
        virtual void CtrlKey(int key) = 0;
        virtual void AltKey(int key) = 0;
        virtual void CtrlAltKey(int key) = 0;
        virtual void CtrlShiftKey(int key) = 0;
        virtual void AltShiftKey(int key) = 0;
        virtual void CtrlAltShiftKey(int key) = 0;
        // Реф. PanelKeySimulation: сообщение 0x41A в очередь KSrvBaseThread.
        virtual void PanelKey(int key, int event, int value) = 0;
    };
    static void SetKeySink(IKeySink *sink);   // не из реф.: шов вместо uinput/IPC

    explicit KAutoTestThread(QObject *parent = nullptr);

    // ⚠️ Реф. ПЕРЕОПРЕДЕЛЯЕТ невиртуальный QThread::start: если поток уже бежит — выход;
    // иначе снимается флаг остановки, поднимается uinput-устройство и зовётся QThread::start.
    void start(Priority priority = InheritPriority);
    void stop();                              // реф. @0x6fc290: флаг + Remove_uinput_device

    void ResetFileExecCount();                // реф. @0x6fb4c8: {+0x14, +0x18} = {1, 0}
    void SetLogCheckOpen(bool open);          // реф. @0x6fc340: поле +0x124
    bool IsLogCheckOpen() const { return m_bLogCheckOpen; }
    int  FileExecCount() const { return m_nFileExecCount; }

    // Реф. @0x6fc348: <LogPath>/APPlog%04d-%02d.txt по ТЕКУЩИМ году и месяцу.
    QString GetLogPath() const;

    // Реф. @0x6fc818 — читает глобальный статус автотеста: истина при значении,
    // отличном И от 0, И от 2 (`tst w0,#0xfffffffd`).
    static bool IsAutoTestStart();

    // Реф. @0x6fc558: младшие 28 бит — код клавиши Qt, старшие — модификаторы
    // (бит 29 = Shift, 30 = Ctrl, 31 = Alt; та же раскладка, что в KEY_CONFIG скрипта).
    void KeyboardSimulation(int param);
    // Реф. @0x6fc2a0: собирает сообщение 0x41A {key, (value>>16)&0xff, value&0xff}.
    void PanelKeySimulation(int key, int value);

    // Не из реф.: точка наполнения очереди (в реф. её наполняет RecvMsg с типом 1).
    void EnqueueKey(int key, int event);
    int  PendingKeys() const { return m_keyQueue.size(); }
    // Один оборот тела run() без потока — для self-test.
    void ProcessPendingKeys();

signals:
    void DisplayMsg(QString msg);       // реф. @0x8306d0
    void ShowMsg(QString msg);          // реф. @0x830728
    void TestStatusChange(int status);  // реф. @0x8306f8

protected:
    void run() override;                // реф. @0x6fc618

private:
    bool m_bStop = false;               // +0x10
    int  m_nFileExecCount = 0;          // +0x14 (ResetFileExecCount → 1)
    int  m_nExecIndex = 0;              // +0x18
    bool m_bLogCheckOpen = false;       // +0x124
    QQueue<QPair<int, int>> m_keyQueue; // в реф. — глобальная std::deque<{key,event}>
};

// Реф. свободная функция-акцессор @0x6fc830.
KAutoTestThread *GetKAutoTestThread();
