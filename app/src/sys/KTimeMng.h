#pragma once

#include <QObject>
#include <QTime>

#include <functional>

class QTimer;
class KUiMsgProxy;

// Часы и суточное обслуживание прибора (реф. KTimeMng @0x458378, X-2600).
// Синглтон — доступ ТОЛЬКО через свободную GetKTimeMng() @0x458800 (ленивое new,
// sizeof 0x30, без Instance()-метода и без удаления при выходе).
//
// Конструктор сразу зовёт InitTimer() и заводит ТРИ таймера (все с parent=nullptr,
// как в реф. — владения нет, живут до конца процесса):
//   • 1000 мс → Timedisplay() — часы в шапке экрана;
//   • 60000 мс → EachDayMC()  — суточный тик, работает только в 00:00;
//   • 1500 мс → InitMC()      — ОДНОРАЗОВЫЙ отложенный старт контроля наработки
//                               (сам себя останавливает в первом же срабатывании).
// Четвёртой связи нет: секундомер записи подключается/отключается ОТДЕЛЬНО через
// StartRecTimer()/StopRecTimer() — они цепляют timeout() того же 1-секундного
// таймера прямо к сигналу KUiMsgProxy::UpdateRecTime() (сигнал→сигнал).
class KTimeMng : public QObject
{
    Q_OBJECT
public:
    explicit KTimeMng(QObject *parent = nullptr);
    ~KTimeMng() override;

    void InitTimer();        // реф. @0x458160
    void StartRecTimer();    // реф. @0x458100 — connect(timer1, timeout, proxy, UpdateRecTime)
    void StopRecTimer();     // реф. @0x458148 — disconnect того же

    // Не из реф.: шов для self-test — подмена «текущего времени» для EachDayMC.
    // На приборе всегда QTime::currentTime().
    static void SetTimeProvider(std::function<QTime()> f);

public slots:
    void Timedisplay();      // реф. @0x4583d0
    void EachDayMC();        // реф. @0x4586a8
    void InitMC();           // реф. @0x4587d8

private:
    KUiMsgProxy *m_pUiMsgProxy = nullptr;   // реф. поле +0x10 (= GetKUiMsgProxy())
    QTimer *m_pTimer = nullptr;             // +0x18, 1 с
    QTimer *m_pDayTimer = nullptr;          // +0x20, 1 мин
    QTimer *m_pInitTimer = nullptr;         // +0x28, 1.5 с (одноразовый)
};

// Реф. свободная функция-акцессор @0x458800.
KTimeMng *GetKTimeMng();
