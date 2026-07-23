#pragma once

#include <QDialog>

class QGridLayout;
class QLabel;
class QProgressBar;
class QTimer;

// Экран выключения прибора (реф. KPoweroff : QDialog, ctor @0x626620, sizeof 0x40).
// UI-порт. Точка входа — свободная функция SystemPoweroff() @0x626d18 (new → exec → delete).
//
// Раскладка (uic-стиль, все константы из ctor): resize 1280×1024; QGridLayout "gridLayout"
// с полями (-1,-1,-1,100); строки: спейсер(20×40, Minimum/Expanding) → label_logo 580×120
// фикс, Alignment 0x84 → progressBar 580×30 фикс, value 0 → label_progress 580×60 фикс с
// текстом TR_SDown. → спейсер. Окно: Dialog|FramelessWindowHint|WindowStaysOnTop (0x40803),
// move(0,0) и resize по KSystemSet::GetUIResolution() (device-seam).
//
// ⚠️ ПРОГРЕСС-БАР НЕ ПОКАЗЫВАЕТ РЕАЛЬНЫЙ ПРОГРЕСС: maximum = 20, таймер 50 мс, значение =
// номер тика. На 20-м тике (ровно 1 с) уходит SendToMainCtrl(0) («выключение»), а дальше
// диалог ЖДЁТ, пока не завершатся процессы X2000Video/X2000Simulator/X2000 (CheckProcStatus
// @0x626300 — скан /proc, чистый device-seam), и только тогда закрывается.
class KPoweroff : public QDialog
{
    Q_OBJECT
public:
    explicit KPoweroff(QWidget *parent = nullptr);

    // Реф. @0x626300: все три процесса прибора завершились. Off-device — заглушка (true).
    bool CheckProcStatus() const;

    QProgressBar *Bar() const { return m_progressBar; }
    QTimer *Timer() const { return m_timer; }
    int TickCount() const { return m_count; }

public slots:
    void RepaintProgressBar();   // реф. @0x626358 — тик таймера

signals:
    void powerOffRequested();    // порт: замена SendToMainCtrl(0) на 20-м тике

private:
    void InitDialogParam();      // реф. @0x6263f8

    QGridLayout *m_grid = nullptr;
    QLabel *m_labelLogo = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QLabel *m_labelProgress = nullptr;
    QTimer *m_timer = nullptr;
    int m_count = 0;             // реф. статик-счётчик @0x14be670
};

// Реф. свободная функция @0x626d18.
void SystemPoweroff();
