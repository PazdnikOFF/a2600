#pragma once

#include <QObject>

// Глобальный фильтр нажатий мыши (реф. KGlobalEventFilter : QObject, sizeof 0x18, одно поле
// +0x10). UI-порт. Ctor в реф. ЗАИНЛАЙНЕН в KReportEditUi::InitConnect @0x4d16e0: объект
// создаётся с parent = KReportEditUi, ставится на qApp и коннектится
// SignalGlobalMouseEvent → KReportEditUi::OnGlobalMouseEvent.
//
// eventFilter @0x8228a0: интересует ТОЛЬКО QEvent::MouseButtonPress (тип 2). Далее реф.
// читает 8 байт по смещению QEvent+0x18 и сравнивает с сохранённым значением: совпало —
// молча пропустить, не совпало — испустить сигнал и запомнить. Событие НИКОГДА не съедается
// (всегда возвращается результат QObject::eventFilter).
//
// ⚠️ Что это за 8 байт: в раскладке Qt5 QInputEvent на этом месте лежат modState (int) и
// timestamp (ulong) — то есть ключ дедупликации «модификаторы+момент времени». Смысл:
// ОДНО И ТО ЖЕ нажатие приложение доставляет нескольким объектам, и без ключа сигнал
// испускался бы по нескольку раз на клик. В порте ключ собран явно из modifiers()+timestamp()
// (побайтовое чтение чужой структуры не переносим).
class KGlobalEventFilter : public QObject
{
    Q_OBJECT
public:
    explicit KGlobalEventFilter(QObject *parent = nullptr);

signals:
    void SignalGlobalMouseEvent();   // реф. @0x8221b0 (единственный сигнал класса)

public:
    bool eventFilter(QObject *obj, QEvent *e) override;   // реф. @0x8228a0

private:
    quint64 m_lastKey = 0;   // +0x10 (в реф. — сырые 8 байт события)
};
