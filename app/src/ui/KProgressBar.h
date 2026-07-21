#pragma once

#include <QWidget>
#include <QPixmap>
#include <QVector>

class QLabel;
class QProgressBar;
class QPushButton;
class QTimer;

// Виджет-содержимое индикатора ожидания (реф. KProgressBar : QWidget, Ui_KProgressBar::setupUi
// @0x605090). Встраивается в KWaitProgressBar. Крутилка — 21-кадровый PNG-СПРАЙТ
// (bar/progressbar/progressbar0..20.png из прошивочной темы) по QTimer 200мс; ProgBar скрыт по
// умолчанию (только для процентных задач); строка статуса label_Text; кнопка Cancel.
//
// Реф. worker OperateWaitThread (реальная задача + текст GetDataForShow + _KProgressType) —
// DEVICE-поток, в порт не тянется (WaitStart/SetTaskType — стубы). timerUpdate у нас крутит
// только спрайт (без worker-текста). Сигнал readToClose — по завершении.
class KProgressBar : public QWidget
{
    Q_OBJECT
public:
    explicit KProgressBar(QWidget *parent = nullptr);

    void SetDisplayText(const QString &text);      // реф. @0x604aa8: label_Text
    void SetProgressBarValue(int value);           // реф. @0x604ba0: ProgBar
    void SetBtnCancelVisible(bool visible);        // реф. @0x604bb0: btn_Cancel
    void WaitStart();                              // реф. @0x604a68: старт worker (device-стуб)

signals:
    void readToClose();                            // реф. @0x827fb8

private slots:
    void timerUpdate();                            // реф. @0x604ab8: следующий кадр спрайта
    void OnCancel();                               // реф. @0x6046a8: отмена

private:
    void setupUi();

    QLabel       *label_Text = nullptr;
    QLabel       *label_icomprogress = nullptr;    // крутилка (спрайт)
    QProgressBar *ProgBar = nullptr;               // скрыт по умолчанию
    QPushButton  *btn_Cancel = nullptr;
    QTimer       *timer = nullptr;
    QVector<QPixmap> m_frames;
    int m_frame = 0;
};
