#pragma once

#include <QLabel>
#include <QRect>

class QTimer;

// Авто-скрывающийся тост-попап (реф. KMsgPopup @ctor 0x684788, base QLabel). UI-порт.
// Внутренняя метка m_pLabel (wordwrap, center) + QTimer автоскрытия (5с). Стейт-машина
// m_status (1=idle/2=showing) с очередью одного сообщения. DEVICE-STUB: позиция берётся из
// KDisplayOption::GetSoftEndoViewConf/getVideoRectForUI (над видео-областью) → в порте
// центрируем/фикс-rect.
class KMsgPopup : public QLabel
{
    Q_OBJECT
public:
    explicit KMsgPopup(QWidget *parent = nullptr);

    void Display(const QString &msg, bool flag = false);   // реф. @0x684a08
    void UpdatePostion(const QRect &rect);                  // реф. @0x684638 (device→центр)

private slots:
    void HideLabelInfo();   // реф. @0x684e78: таймаут таймера

private:
    QLabel *m_pLabel = nullptr;   // +0x30
    QTimer *m_pTimer = nullptr;   // +0x38
    QRect m_rect;                 // +0x40
    QString m_pending;            // +0x50
    bool m_flag = false;          // +0x58
    int m_status = 1;             // +0x60 (1 idle / 2 showing)
};
