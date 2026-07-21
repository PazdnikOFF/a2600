#pragma once

#include <QSpinBox>

// Спин ввода возраста (реф. KSpinAge @ctor 0x6904b8, base QSpinBox). UI-порт. Диапазон 1..199,
// стартует ПУСТЫМ (clear). CheckText валидирует на каждый ввод (только положительное; пусто/0/
// отриц → clear/value 0). value() возвращает 0 при пустом; setValue(0)→clear. 100% PORT.
class KSpinAge : public QSpinBox
{
    Q_OBJECT
public:
    explicit KSpinAge(QWidget *parent = nullptr);

    int value() const;                 // 0 при пустом
    void setValue(int v);              // 0 → clear
    void SetPlaceholderText(const QString &t);

protected:
    void keyPressEvent(QKeyEvent *) override;   // Enter в пустом → clear

private slots:
    void CheckText(const QString &text);   // реф. @0x690598
    void EditFinished();                    // реф. @0x690618

private:
    bool m_flag = true;   // +0x30 one-shot
};
