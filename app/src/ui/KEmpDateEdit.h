#pragma once

#include <QDateEdit>
#include <QDate>

// Дата-редактор с ПУСТЫМ (nullable) состоянием (реф. KEmpDateEdit : QDateEdit, ctor @0x715868,
// size 0x48). UI-порт. НЕ кастомная отрисовка — тонкий QDateEdit-подкласс: при m_isEmpty бланчит
// внутренний lineEdit → показывается placeholderText (а не значение). Всё редактирование секций/
// клавиши/клип — штатные QDateEdit. Фокус на пустом поле заполняет дефолт-датой + подсвечивает
// кликнутую секцию; блюр без правки → снова пусто. Sentinel InvalidDate = QDate(2100,1,1) (как
// у [[KDateEdit]]/KPatientDateEdit). 100% PORT (чистый Qt).
class KEmpDateEdit : public QDateEdit
{
    Q_OBJECT
public:
    explicit KEmpDateEdit(QWidget *parent = nullptr);

    static QDate InvalidDate() { return QDate(2100, 1, 1); }   // реф. @0x7160d0
    bool IsEmpty() const { return m_isEmpty; }
    bool CanEmpty() const { return m_canEmpty; }
    void SetEmptyAble(bool able);              // реф. @0x715e60
    void clear() override;                     // реф. @0x715fb0

    QDate date() const;                        // пусто → null QDate
    QTime time() const;
    QDateTime dateTime() const;
    void setDate(const QDate &d);              // валид → un-empty; sentinel+canEmpty → empty
    void setTime(const QTime &t);
    void setDateTime(const QDateTime &dt);

    void SetPlaceholderText(const QString &s);
    void SetCursorPosition(int pos);
    int  LineEditSectionAt(int pos) const;     // реф. @0x716180: формат → Section

protected:
    void paintEvent(QPaintEvent *e) override;
    void showEvent(QShowEvent *e) override;
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    bool focusNextPrevChild(bool next) override;
    QValidator::State validate(QString &input, int &pos) const override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

private slots:
    void DateChangedSlot(const QDate &d);      // реф. @0x7162c0

private:
    void showEmpty();     // пустой показ (порт: specialValueText — как sibling KDateEdit)
    void showValue();     // снять пустое

    bool m_isEmpty = false;   // реф. struct+0x30 байт +8
    bool m_canEmpty = false;  // байт +9
    bool m_guard = false;     // +0x38
    QDate m_defaultDate;      // +0x40
    QString m_placeholder;    // маска пустого (порт)
};
