#pragma once

#include "ui/KDialog.h"

class KProgressBar;

// Модальный индикатор ожидания (реф. KWaitProgressBar : KDialog, ctor @0x609bf8). UI-порт.
// ТОНКАЯ ОБЁРТКА: диалог 480×160, WindowModal, в QGridLayout один дочерний виджет KProgressBar
// (вся крутилка/текст/cancel/таймер — в НЁМ, реф. отдельный класс KProgressBar @0x604a50…,
// Ui_KProgressBar::setupUi @0x605090 — СЛЕДУЮЩИЙ реверс; здесь заглушка QProgressBar
// indeterminate). Своих Start/Stop нет — всё через GetProgressBar(). Esc НЕ закрывает
// (keyPressEvent глотает). Титул TR_Dlg (реф. плейсхолдер).
class KWaitProgressBar : public KDialog
{
    Q_OBJECT
public:
    explicit KWaitProgressBar(QWidget *parent = nullptr);

    // Реф. GetProgressBar() @0x60a038 → KProgressBar*.
    KProgressBar *GetProgressBar() const { return m_progressbar; }

protected:
    void keyPressEvent(QKeyEvent *event) override;   // реф. @0x60ae80: игнорит Esc

private:
    KProgressBar *m_progressbar = nullptr;   // реф. KProgressBar (виджет-содержимое)
};
