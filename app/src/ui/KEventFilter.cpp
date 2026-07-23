#include "KEventFilter.h"

#include <QCoreApplication>
#include <QEvent>
#include <QKeyEvent>

KEventFilter::KEventFilter(QObject *parent)
    : QObject(parent)
{
    // Реф. ctor @0x67a960: сам ставит себя фильтром НА ПРИЛОЖЕНИЕ.
    if (QCoreApplication::instance())
        QCoreApplication::instance()->installEventFilter(this);
}

KEventFilter &KEventFilter::CreateInstance()
{
    // Реф. @0x67a9e0: function-local static, parent = nullptr.
    static KEventFilter inst;
    return inst;
}

void KEventFilter::ScreenShotSave()
{
    // Реф. @0x67aa60: GetKUiMsgProxy()->SendToMainCtrl(5) — и БОЛЬШЕ НИЧЕГО.
    emit screenShotRequested();
}

bool KEventFilter::eventFilter(QObject *obj, QEvent *e)
{
    const QEvent::Type t = e->type();
    // Реф.: `sub w0,#2; cmp w0,#5; b.hi` — диапазон типов 2..7 включительно.
    if (t >= QEvent::MouseButtonPress && t <= QEvent::KeyRelease) {
        emit userActivity();   // реф. KForceLogout::Instance().RefreshActiveTime()

        if (t == QEvent::KeyPress) {
            auto *ke = static_cast<QKeyEvent *>(e);
            // Реф.: ТОЧНОЕ равенство модификаторов, а не проверка бита.
            const bool ctrlOnly = ke->modifiers() == Qt::ControlModifier;
            // 0x50 = Qt::Key_P; 0x417 = «З» (та же клавиша в русской раскладке).
            if (ctrlOnly && (ke->key() == 0x50 || ke->key() == 0x417)) {
                ScreenShotSave();
                return true;   // событие съедается
            }
        }
    }
    return QObject::eventFilter(obj, e);
}
