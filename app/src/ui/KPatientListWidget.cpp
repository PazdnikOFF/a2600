#include "KPatientListWidget.h"

KPatientListWidget::KPatientListWidget(QWidget *parent)
    : QListWidget(parent)
{
    // Реф. ctor @0x79e5d8: тривиальный QListWidget; стиль/строки — в InitListWidgetItem оболочки.
    setObjectName(QStringLiteral("listwidget"));
}
