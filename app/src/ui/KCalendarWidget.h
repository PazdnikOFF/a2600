#pragma once

#include <QCalendarWidget>

// Кастомный date-picker (реф. KCalendarWidget @ctor 0x7af690). УСТАНОВЛЕНО реверсом: это
// ВЕРБАТИМ-форк исходника Qt5 QCalendarWidget с переименованием (QCalendarWidget→KCalendarWidget,
// QCalendarView→KCalendarView, модель — стоковая QCalendarModel). Поведение — стоковое Qt,
// кроме косметики нав-бара (месяц — QComboBox, год — QSpinBox вместо menu-button). Реф.-путь
// (рекомендация реверса): подкласс стокового QCalendarWidget — идентичные API/сигналы
// (clicked(QDate)/selectionChanged/activated, selectedDate/setSelectedDate/setDateRange/
// setMinimumDate). Косметическое отличие нав-бара опущено (стоковый нав-бар функционально
// эквивалентен). Попап-поведение — задача ВЛАДЕЛЬЦА (форма показывает как Qt::Popup под полем,
// коннектит clicked(QDate)→setDate поля). 100% PORT (только KSystemSet::GetSystemLanguage для
// локали + QDate::currentDate — host-safe).
class KCalendarWidget : public QCalendarWidget
{
    Q_OBJECT
public:
    explicit KCalendarWidget(QWidget *parent = nullptr);
};
