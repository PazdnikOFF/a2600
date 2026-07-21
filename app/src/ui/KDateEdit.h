#pragma once

#include <QDateEdit>
#include <QString>

// Дата-эдит с бланк-маской-плейсхолдером (реф. KDateEdit @ctor 0x813600, base QDateEdit).
// UI-порт. Пустое = specialValueText == m_placeholder («  /  /    »); сентинел пустого для
// date() — InvalidDate=QDate(2100,1,1). ТОТ ЖЕ механизм, что у KPatientDateEdit, но плейсхолдер
// — фикс. бланк-маска (не формат) + toggle specialValueText⇄«» чтобы date() различала set/unset
// + focus-out возврат к бланку если не закоммичено. 100% PORT.
class KDateEdit : public QDateEdit
{
    Q_OBJECT
public:
    explicit KDateEdit(QWidget *parent = nullptr);

    static QDate InvalidDate() { return QDate(2100, 1, 1); }   // реф. @0x8139a0
    QDate date() const;                        // реф. @0x8139c8: пусто → InvalidDate
    void setDate(const QDate &d);              // реф. @0x813ca8
    void clear();                              // реф. @0x813c68: назад в бланк
    void setDisplayFormat(const QString &f);   // реф. @0x813728
    void SetPlaceholderText(const QString &t) { m_placeholder = t; }

protected:
    void focusOutEvent(QFocusEvent *) override;   // реф. @0x813f88: revert к бланку

private slots:
    void DateChangedSlot(const QDate &d);         // реф. @0x813a98

private:
    bool isEmpty() const { return specialValueText() == m_placeholder; }

    QString m_placeholder = QStringLiteral("  /  /    ");   // +0x30
    bool m_edited = false;   // +0x40
};
