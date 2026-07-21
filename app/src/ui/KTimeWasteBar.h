#pragma once

#include "KDialog.h"
#include <QTimer>

class QLabel;
class QProgressBar;

// Модальный «пожалуйста, подождите» с авто-заполняющимся прогрессом (реф. KTimeWasteBar
// @ctor 0x6095b0, base KDialog — НЕ QWidget). UI-порт. За nSeconds секунд бар едет 0→100%
// (QTimer 100мс, m_nMax=nSeconds*10 шагов), на 100% → close(). resize(480,160), SetKStyle(4)=
// W480, title TR_Dlg. QGridLayout: topSpacer / label_Text(TR_PWait, статичный) / ProgBar
// (0..100, «%p%») / bottomSpacer. Esc закрывает только при SetEscEnable(true). Своих сигналов
// НЕТ (завершение = close → QDialog::finished). 100% PORT.
class KTimeWasteBar : public KDialog
{
    Q_OBJECT
public:
    explicit KTimeWasteBar(QWidget *parent = nullptr, int nSeconds = 10);

    void SetCount(int n) { m_nCount = n; }         // реф. @0x609510
    void SetEscEnable(bool b) { m_bEscEnable = b; } // реф. @0x609508

protected:
    void keyPressEvent(QKeyEvent *) override;       // реф. @0x609518: Esc→close если разрешён

private slots:
    void TimeSlot();   // реф. @0x609538: тик прогресса

private:
    QLabel *m_label = nullptr;
    QProgressBar *m_progBar = nullptr;
    QTimer m_timer;
    int m_nCount = 0;    // +0x78
    int m_nMax = 100;    // +0x7c
    bool m_bEscEnable = false;   // +0x80
};
