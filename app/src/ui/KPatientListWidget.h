#pragma once

#include <QListWidget>

// Лево-навигационное меню режимов управления пациентами (реф. KPatientListWidget @ctor
// 0x79e5d8, base QListWidget). UI-порт. Тривиальный подкласс QListWidget — наполнение и
// стиль задаёт оболочка KPatientManagmentUi::InitListWidgetItem (чёрный фон rgb(26,26,26),
// gridSize 215×103, скроллбары off, строки = режимы Patient/Case/DICOM). Строки-виджеты в
// реф. — KPatientListWidgetItem (иконка+метка, состояния Select/UnSelect/Hover/Disable); в
// порте упрощены до штатных QListWidgetItem (иконка+текст) — навигационная функция сохранена.
class KPatientListWidget : public QListWidget
{
    Q_OBJECT
public:
    explicit KPatientListWidget(QWidget *parent = nullptr);
};
