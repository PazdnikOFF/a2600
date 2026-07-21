#pragma once

#include <QWidget>
#include <QStringList>

class QVBoxLayout;
class QLabel;

// Плавающий оверлей-список сообщений (реф. KFloatingMsg @ctor 0x68fb48, base QWidget). UI-порт.
// Показывает список сообщений, склеенных «\r\n», в центрированной метке label_FloatingM.
// Сообщения добавляются/удаляются явно (без таймера/автоскрытия). Фон чёрный. 100% PORT.
class KFloatingMsg : public QWidget
{
    Q_OBJECT
public:
    explicit KFloatingMsg(QWidget *parent = nullptr);

    void AddText(const QString &text);      // реф. @0x690030: если нет — append + ShowText
    void DelText(const QString &text);      // реф. @0x68f898
    void SetTextShow(const QString &text, bool show);   // реф. @0x6900f0: show?Add:Del
    void ClearTextShow();                    // реф. @0x690008
    QStringList GetTextList() const { return m_textList; }
    void SetFloatingRect(const QRect &rect); // реф. @0x68f540: move + resize

private:
    void ShowText();   // реф. @0x68f5c0: join «\r\n» → label

    QVBoxLayout *m_layout = nullptr;
    QLabel *m_label = nullptr;   // label_FloatingM
    QStringList m_textList;      // +0x38
};
