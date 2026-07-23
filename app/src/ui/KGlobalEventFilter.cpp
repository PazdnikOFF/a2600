#include "KGlobalEventFilter.h"

#include <QCoreApplication>
#include <QEvent>
#include <QMouseEvent>

KGlobalEventFilter::KGlobalEventFilter(QObject *parent)
    : QObject(parent)
{
    // Реф. (инлайн в KReportEditUi::InitConnect @0x4d24e0): поле = 0, затем фильтр на qApp.
    if (QCoreApplication::instance())
        QCoreApplication::instance()->installEventFilter(this);
}

bool KGlobalEventFilter::eventFilter(QObject *obj, QEvent *e)
{
    if (e->type() != QEvent::MouseButtonPress)
        return QObject::eventFilter(obj, e);

    auto *me = static_cast<QMouseEvent *>(e);
    // Реф. ключ дедупликации — 8 байт QEvent+0x18 (modState + timestamp); собираем явно.
    const quint64 key = (quint64(me->timestamp()) << 32) | quint32(me->modifiers());
    if (key == m_lastKey)
        return QObject::eventFilter(obj, e);   // тот же клик, повторно доставленный

    emit SignalGlobalMouseEvent();
    m_lastKey = key;
    return QObject::eventFilter(obj, e);       // реф. событие НЕ съедает
}
