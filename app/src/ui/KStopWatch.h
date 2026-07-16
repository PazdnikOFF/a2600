#pragma once

#include <QFrame>
#include <QTime>

class QLabel;
class QTimer;

// Экранный секундомер процедуры (реф. KStopWatch : QFrame, X-2600). НЕ device:
// это виджет отображения + конечный автомат, тик — целые секунды по QTimer(1000мс)
// (реального монотонного таймера нет — счётчик тиков, 1:1 с оригиналом).
// Потребитель сигнала — KUiMsgProxy::StopWatchStateChanged → KUIDesktop/KSystemStatus.
class KStopWatch : public QFrame
{
    Q_OBJECT
public:
    // Реф. STOPWATCH_STATUS: 0 = стоп/сброс, 1 = пауза, 2 = ход.
    enum STOPWATCH_STATUS { Stop = 0, Pause = 1, Run = 2 };

    explicit KStopWatch(QWidget *parent = nullptr);
    ~KStopWatch() override;

    STOPWATCH_STATUS State() const { return m_state; }
    QString TimeText() const;   // текст timelabel

    // Начальная инициализация 00:00:00 (в реф. её делает внешний KViewBase::InitStopWatch;
    // вынесена сюда, т.к. ctor реф. оставляет m_time невалидным — это компенсируется снаружи).
    void InitStopWatch();

public slots:
    void UpdateTime();                   // слот тика: m_time += 1 сек
    void HandleKeyPress(int key);        // Space(0x20)→Pause, F1(0x01000030)→Run
    void HandleStopWatchRuningState();   // state 0→Run (старт); иначе → Stop (стоп+сброс)
    void HandleStopWatchPauseState();    // Pause⇄Run; при Stop ничего

signals:
    void StopWatchStateChanged(int status);

private:
    QLabel *m_pixmapLabel = nullptr;
    QLabel *m_timeLabel   = nullptr;
    QTime   m_time;                       // +0x38 (мс-с-полуночи)
    STOPWATCH_STATUS m_state = Stop;      // +0x3c
    QTimer *m_pTimer = nullptr;           // +0x40
};
