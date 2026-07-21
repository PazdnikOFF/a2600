#pragma once

#include <QFrame>
#include <QList>
#include <QString>

class QLabel;
class QVBoxLayout;

// Динамический 5-строчный лог сообщений (реф. KMessageFrame @ctor 0x682ab0, base QFrame,
// Ui_KMessageFrame::setupUi @0x683750). UI-порт. Пассивный дисплей: 5 QLabel в QVBoxLayout,
// наполняется извне через AddShowMessage/ClearShowMessage. Каждое сообщение несёт count
// (обратный отсчёт жизни), уменьшаемый ShowMessageCheck (тик гонит ВЛАДЕЛЕЦ по таймеру).
// Сигналов/device НЕТ.
class KMessageFrame : public QFrame
{
    Q_OBJECT
public:
    explicit KMessageFrame(QWidget *parent = nullptr);

    void AddShowMessage(const QString &msg, int count);   // реф. @0x683020: дедуп+append+автоскролл
    void ClearShowMessage(const QString &msg);            // реф. @0x683630
    void UpdateShowMessage();                             // реф. @0x682be0: перестроить 5 меток
    void ShowMessageCheck();                              // реф. @0x683228: тик — countdown+скролл

private:
    void setupUi();

    struct KShowMessage { QString text; int count; };   // реф. _KShowMessage (16 байт)

    QLabel *m_labels[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
    QList<KShowMessage> m_messages;   // +0x38
    int m_startIndex = 0;             // +0x40
};
