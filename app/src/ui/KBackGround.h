#pragma once

#include "KDialog.h"

class QTimer;

// Пустой фоновый диалог-заглушка (реф. KBackGround : KDialog, ctor @0x457cd8, sizeof 0x60).
// UI-порт. Ui-структура пустая (`operator new(1)`), контента нет вообще: фон/пиксмап НЕ
// грузится — ни одного QPixmap в коде класса.
//
// Смысл: фуллскрин-стиль (SetKStyle(1)) без кнопки закрытия, который САМ закрывается через
// QTimer с интервалом 1000 мс (connect timeout → close). Реф. точка входа — свободная
// функция OpenBackGround() @0x457fe8 (new → exec → delete), но ⚠️ вызовов OpenBackGround в
// бинарнике НЕТ: экран собран, но нигде не открывается (мёртвый экспорт).
class KBackGround : public KDialog
{
    Q_OBJECT
public:
    explicit KBackGround(QWidget *parent = nullptr);

    QTimer *Timer() const { return m_timer; }   // для проверок (в реф. — поле +0x58)

private:
    QTimer *m_timer = nullptr;
};

// Реф. свободная функция @0x457fe8 (в прошивке не вызывается ниоткуда).
void OpenBackGround();
