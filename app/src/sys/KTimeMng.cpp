#include "sys/KTimeMng.h"

#include "sys/KSystemSet.h"
#include "sys/KSystemStatus.h"
#include "ui/KUiMsgProxy.h"

#include <QDateTime>
#include <QTimer>

namespace {
// Шов self-test (не из реф.): по умолчанию — системное время прибора.
std::function<QTime()> g_timeProvider = []{ return QTime::currentTime(); };
}

void KTimeMng::SetTimeProvider(std::function<QTime()> f)
{
    g_timeProvider = f ? f : std::function<QTime()>([]{ return QTime::currentTime(); });
}

KTimeMng *GetKTimeMng()
{
    // Реф.: статический указатель, ленивое `new KTimeMng(nullptr)`, delete НИКОГДА.
    static KTimeMng *inst = nullptr;
    if (!inst)
        inst = new KTimeMng(nullptr);
    return inst;
}

KTimeMng::KTimeMng(QObject *parent)
    : QObject(parent)
    , m_pUiMsgProxy(GetKUiMsgProxy())
{
    InitTimer();
}

KTimeMng::~KTimeMng() = default;

void KTimeMng::InitTimer()
{
    // ⚠️ Реф. создаёт таймеры БЕЗ родителя (`QTimer(nullptr)`) и не удаляет их —
    // объект-синглтон живёт до конца процесса. Повторяем, но с this как родителем
    // было бы безопаснее; оставляем как в реф., чтобы поведение при удалении
    // KTimeMng совпадало (таймеры продолжают жить).
    m_pTimer = new QTimer(nullptr);
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(Timedisplay()));
    m_pTimer->start(1000);

    m_pDayTimer = new QTimer(nullptr);
    connect(m_pDayTimer, SIGNAL(timeout()), this, SLOT(EachDayMC()));
    m_pDayTimer->start(60000);

    m_pInitTimer = new QTimer(nullptr);
    connect(m_pInitTimer, SIGNAL(timeout()), this, SLOT(InitMC()));
    m_pInitTimer->start(1500);
}

void KTimeMng::StartRecTimer()
{
    // Реф.: connect(m_pTimer, SIGNAL(timeout()), m_pUiMsgProxy, SIGNAL(UpdateRecTime()))
    // — секундомер записи тикает от ТОГО ЖЕ 1-секундного таймера, что и часы.
    connect(m_pTimer, SIGNAL(timeout()), m_pUiMsgProxy, SIGNAL(UpdateRecTime()));
}

void KTimeMng::StopRecTimer()
{
    disconnect(m_pTimer, SIGNAL(timeout()), m_pUiMsgProxy, SIGNAL(UpdateRecTime()));
}

void KTimeMng::Timedisplay()
{
    // Реф.: GetKSystemSetStatus()->формат_даты + "\nhh:mm:ss" (перевод строки —
    // дата и время выводятся в две строки), затем toString текущего QDateTime.
    const QString format = KSystemSet::GetInstance().DateFormat() + QLatin1String("\nhh:mm:ss");
    emit m_pUiMsgProxy->UpdateSystemtime(QDateTime::currentDateTime().toString(format));
}

void KTimeMng::EachDayMC()
{
    // Таймер тикает раз в минуту, но работа делается только в полночь.
    const QTime now = g_timeProvider();
    if (now.hour() != 0 || now.minute() != 0)
        return;

    emit m_pUiMsgProxy->CheckMachineControl();

    // ⚠️ Квирк реф.: GetRemainDays().toInt() вычисляется ДВАЖДЫ, первый результат
    // отбрасывается. Побочных эффектов у чтения нет, поэтому у нас один вызов.
    if (KSystemSet::GetInstance().GetRemainDays().toInt() == 1) {
        // Последний день лицензии истекает сегодня — снимаем флаг авторизации.
        KSystemSet::GetInstance().SetProductAuthFlag(0);
        emit KSystemStatus::GetInstance().AuthMachineChange();
    }
}

void KTimeMng::InitMC()
{
    // Одноразовый отложенный старт: дёрнуть контроль наработки и заглушить свой таймер.
    emit m_pUiMsgProxy->CheckMachineControl();
    m_pInitTimer->stop();
}
