#pragma once

#include <QFrame>

// Гравированные разделители (реф. KLineH @0x68c8e8 / KLineV @0x68c7d8, base QFrame).
// UI-порт РЕАЛЬНЫХ кастом-виджетов — ранее подставлялись обычным QFrame(HLine/Sunken)
// в ~8 диалогах. У обоих НЕТ методов/сигналов: ctor делает ровно 3 вызова —
// setStyleSheet(<линейный градиент>) + min/max по фикс-оси (2px). Толщина/цвета
// зашиты в ctor. Эффект «утопленной канавки» (источник света сверху/слева):
//   KLineH: 2px высотой, верх #010101 → низ #3B3B3B (жёсткий стык на середине);
//   KLineV: 2px шириной, слева #3B3B3B → справа #010101.
// 100% PORT: чистый Qt, никаких device-зависимостей.

class KLineH : public QFrame
{
    Q_OBJECT
public:
    explicit KLineH(QWidget *parent = nullptr);
};

class KLineV : public QFrame
{
    Q_OBJECT
public:
    explicit KLineV(QWidget *parent = nullptr);
};
