#pragma once

#include <QDateEdit>
#include <QString>

// Дата-эдит с пустым/placeholder-состоянием (реф. KPatientDateEdit @ctor 0x7b7e88, base
// QDateEdit). UI-порт РЕАЛЬНОГО кастом-виджета — ранее подставлялся голым QDateEdit
// (KPatientListSearch/KPatientListAddDlg). Пустое состояние — через specialValueText:
// когда дата == minimumDate, показывается m_format как плейсхолдер («MM/DD/YYYY»). Сентинел
// «пусто» для date() — QDate(2100,1,1). Цвет текста: серый #999 (плейсхолдер) / тёмный
// #3D3D3D (значение). Клик по пустому полю авто-сеет сегодняшнюю дату. 100% PORT (QDate::
// currentDate — системные часы, не device).
class KPatientDateEdit : public QDateEdit
{
    Q_OBJECT
public:
    explicit KPatientDateEdit(QWidget *parent = nullptr);

    QDate date() const;                        // реф. @0x7b8160: пусто → InvalidDate
    void setDate(const QDate &d);              // реф. @0x7b85e0
    void setDisplayFormat(const QString &f);   // реф. @0x7b8018: m_format=upper, плейсхолдер
    void clear();                              // реф. @0x7b85a0: назад в плейсхолдер
    static QDate InvalidDate() { return QDate(2100, 1, 1); }   // реф. @0x7b8138
    void SetPlaceholderText(const QString &t);
    void SetTextMargins(int l, int t, int r, int b);

public slots:
    // Показать KCalendarWidget попапом под полем; выбор → setDate (реф. владелец так
    // связывает calendar-кнопку). Реюзабельный хук интеграции с портом KCalendarWidget.
    void PopupCalendar();

protected:
    void focusInEvent(QFocusEvent *) override;
    void focusOutEvent(QFocusEvent *) override;
    void keyPressEvent(QKeyEvent *) override;

private slots:
    void DateChangedSlot(const QDate &d);          // реф. @0x7b8230
    void SlotToChangeTextColor(const QString &t);  // реф. @0x7b7cc8

private:
    bool isEmpty() const;   // specialValueText()==m_format
    void applyTextColor();

    QString m_format = QStringLiteral("MM/DD/YYYY");   // +0x30 (upper, = плейсхолдер)
    bool m_pendingClear = false;                        // +0x38
};
