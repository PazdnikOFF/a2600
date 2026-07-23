#pragma once

#include <QObject>

// Глобальный фильтр событий приложения (реф. KEventFilter : QObject, ctor @0x67a960,
// sizeof 0x10 — своих полей НЕТ). UI-порт.
//
// Ctor САМ ставит себя на qApp (`qApp->installEventFilter(this)`), а CreateInstance()
// @0x67a9e0 — function-local static (создаётся при первом вызове, parent = nullptr);
// зовётся ровно один раз из KAppThread::start() @0x626098.
//
// eventFilter @0x67aa78 делает две вещи:
//  1) на ЛЮБОМ пользовательском событии (тип 2..7 = MouseButtonPress/Release/DblClick/
//     MouseMove/KeyPress/KeyRelease) дёргает KForceLogout::RefreshActiveTime() — сброс
//     таймера авто-выхода (DEVICE-seam, у нас — сигнал userActivity);
//  2) горячая клавиша скриншота: modifiers() == Qt::ControlModifier (ТОЧНОЕ равенство
//     0x04000000, не побитовая проверка!) и key() == 0x50 (Qt::Key_P) ЛИБО key() == 0x417
//     (U+0417 «З» — та же физическая клавиша в русской раскладке). Событие съедается.
//
// ScreenShotSave @0x67aa60 — DEVICE-seam: только KUiMsgProxy::SendToMainCtrl(5); сам
// снимок делает главный контроллер, путь/формат в этом классе НЕ ВОССТАНАВЛИВАЮТСЯ.
class KEventFilter : public QObject
{
    Q_OBJECT
public:
    explicit KEventFilter(QObject *parent = nullptr);

    static KEventFilter &CreateInstance();   // реф. @0x67a9e0 — синглтон по первому вызову

    void ScreenShotSave();   // реф. @0x67aa60 → SendToMainCtrl(5), в порте — сигнал

signals:
    void screenShotRequested();   // порт: замена SendToMainCtrl(5)
    void userActivity();          // порт: замена KForceLogout::RefreshActiveTime()

public:
    bool eventFilter(QObject *obj, QEvent *e) override;   // реф. @0x67aa78
};
