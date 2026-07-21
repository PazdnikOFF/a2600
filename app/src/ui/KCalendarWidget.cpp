#include "KCalendarWidget.h"

KCalendarWidget::KCalendarWidget(QWidget *parent)
    : QCalendarWidget(parent)
{
    // Реф. ctor @0x7af690: форк QCalendarWidget. Стоковое поведение + локаль из системы.
    // Косметика (Highlight-band нав-бара, month-combo, year-spin) — у стокового виджета есть
    // функциональные эквиваленты; специальную настройку опускаем. GridVisible off (как Qt-деф).
    setGridVisible(false);
    setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);   // без номера недели (компактнее)
}
