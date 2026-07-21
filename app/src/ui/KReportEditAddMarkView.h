#pragma once

#include <QWidget>

// Панель добавления меток в отчёт (реф. KReportEditAddMarkView : QWidget — НЕ диалог!,
// ctor @0x4be7b8, Ui_KReportEditAddMarkView::setupUi @0x4c16e0). UI-порт. Встраиваемый
// виджет (реф. 1614×760), кладётся KReportEditUi в grid(0,1); своих OK/Cancel НЕТ.
// Порт как самостоятельный QWidget (базовый класс совпал). Три колонки:
//   A (метка изображения): label TR_IProcessing + 5 arrow-radio (lock/ld/lu/ru/rd,
//     эксклюзив) + graphicsView_imgMark + label_file_name + Prev/Next/Clear;
//   B (имя позиции): label TR_OName + label_selected + listWidget_organ (editable,
//     device) + Add/Del/Clear(disabled);
//   C (боди-метка): label TR_Bdmk + 5 radio (ld/lu/ru/rd/point, эксклюзив) +
//     graphicsView_positionMark + Clear.
//
// Кастом KImageEditorGraphicsView→QGraphicsView (2 экз.); radio icon-only (иконки —
// ресурсы графики, у нас глиф-замена). Add/Del позиции — операции со списком (UI).
//
// DEVICE в порт не тянется: SetImgMarkArrow*/SetPositionMark* (движок графики),
// draw-event ответы, img-нав/clear, listWidget-персист (KReportEditDataSource),
// label_file_name/selected — заглушки.
class KReportEditAddMarkView : public QWidget
{
    Q_OBJECT
public:
    explicit KReportEditAddMarkView(QWidget *parent = nullptr);

private:
    void setupUi();
};
