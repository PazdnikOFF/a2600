#pragma once

#include <QLineEdit>

// Поле ввода номера страницы пейджера (реф. KPageLineEdit @ctor 0x7b6990, base QLineEdit).
// UI-порт РЕАЛЬНОГО кастом-виджета. НЕ контейнер — только editable-бокс текущей страницы
// («N/M total» рисует пейджер-бар отдельно). Валидатор ограничивает ввод диапазоном [1,max].
// m_value (+0x30) — закоммиченная текущая страница (источник истины).
//
// РЕФ. использует сгенерированный range-regex GetRegFromNumber(max) в QRegExpValidator; в
// порте — QIntValidator(1,max) (реверс подтвердил функциональную эквивалентность; точную
// побайтовую regex-генерацию не воспроизводим). Собственных сигналов НЕТ — владелец слушает
// штатные returnPressed/editingFinished и коммитит обратно через setText. Реф. mouse-оверрайды
// (drag-select для тачскрина) покрыты штатным QLineEdit. 100% PORT.
class KPageLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit KPageLineEdit(QWidget *parent = nullptr);

    void SetMaximumValue(int max);      // реф. @0x7b6b20: пересобрать валидатор [1,max]
    void setText(const QString &text);  // реф. @0x7b57e0: обновить m_value + база
    int Value() const { return m_value; }

protected:
    void focusOutEvent(QFocusEvent *) override;   // реф. @0x7b6c68: revert к m_value

private:
    int m_value = 1;   // +0x30
};
