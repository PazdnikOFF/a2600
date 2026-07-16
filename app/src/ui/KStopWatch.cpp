#include "ui/KStopWatch.h"

#include <QFont>
#include <QLabel>
#include <QTimer>

namespace {
const char *TIME_FMT = "hh:mm:ss";
}

KStopWatch::KStopWatch(QWidget *parent) : QFrame(parent)
{
    resize(190, 30);   // реф. геометрия

    m_pixmapLabel = new QLabel(this);
    m_pixmapLabel->setObjectName("pixmaplabel");
    m_pixmapLabel->setGeometry(10, 0, 10, 30);

    m_timeLabel = new QLabel(this);
    m_timeLabel->setObjectName("timelabel");
    QFont f = m_timeLabel->font();
    f.setWeight(75);   // реф. bold-ish
    m_timeLabel->setFont(f);
    m_timeLabel->setGeometry(30, 0, 150, 30);

    setWindowTitle(tr("Form"));

    m_pTimer = new QTimer(this);
    connect(m_pTimer, &QTimer::timeout, this, &KStopWatch::UpdateTime);

    // Реф. ctor оставляет m_time невалидным (обнуляет внешний InitStopWatch) —
    // делаем это здесь, чтобы виджет был самодостаточен.
    InitStopWatch();
}

KStopWatch::~KStopWatch() = default;

void KStopWatch::InitStopWatch()
{
    m_state = Stop;
    m_time = QTime(0, 0, 0, 0);
    if (m_pTimer)
        m_pTimer->stop();
    if (m_timeLabel)
        m_timeLabel->setText(m_time.toString(TIME_FMT));
}

QString KStopWatch::TimeText() const
{
    return m_timeLabel ? m_timeLabel->text() : QString();
}

void KStopWatch::UpdateTime()
{
    m_time = m_time.addSecs(1);   // реф. — ровно +1 секунда за тик
    m_timeLabel->setText(m_time.toString(TIME_FMT));
}

void KStopWatch::HandleKeyPress(int key)
{
    if (key == 0x20)                    // Qt::Key_Space
        HandleStopWatchPauseState();
    else if (key == 0x01000030)         // Qt::Key_F1
        HandleStopWatchRuningState();
    // прочие клавиши — реф. ничего
}

void KStopWatch::HandleStopWatchRuningState()
{
    if (m_state == Stop) {
        m_state = Run;
        m_pTimer->start(1000);
        emit StopWatchStateChanged(m_state);
    } else {
        // Стоп + сброс (реф.).
        m_state = Stop;
        m_pTimer->stop();
        m_time = QTime(0, 0, 0, 0);
        m_timeLabel->setText(m_time.toString(TIME_FMT));
        emit StopWatchStateChanged(m_state);
    }
}

void KStopWatch::HandleStopWatchPauseState()
{
    if (m_state == Pause) {
        m_state = Run;
        m_pTimer->start(1000);   // resume
    } else if (m_state == Run) {
        m_state = Pause;
        m_pTimer->stop();        // pause
    }
    // state == Stop → реф. ничего
}
