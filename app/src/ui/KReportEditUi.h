#pragma once

#include "ui/KDialog.h"

// Редактор отчёта обследования (реф. KReportEditUi : KFullScreenDialog — НЕ KDialog!,
// ctor @0x4d4108, Ui_KReportEditUi::setupUi @0x4d7998). UI-порт. Полноэкранная страница
// 1920×1195 (реф. base KFullScreenDialog(parent,2007)); портируем над KDialog(FULLSCREEN).
// Слева колонка кнопок (выбор изображения/метка/заключение | DICOM/печать/шаблон/
// глоссарий/настройки | Save/Exit, все 211px). Справа: сетка базовой инфо (ENo/дата/
// эндоскоп/статус, имя/пол+возраст/PID/направитель, ДР/тел/койка/польз-поля) +
// контейнер изображений (runtime) + 8 текстовых полей (осмотр/заключение/болезнь/метод/
// операц.находки/рекомендации/2 польз-поля) со счётчиками n/max + ряд врачей
// (биопсия/HP/ассистент/врач) + панель глоссария (runtime).
//
// Кастом заменены: KFullScreenDialog→KDialog(FULLSCREEN); KGridWidget→QWidget+grid;
// KQuickInputComboBox→editable QComboBox; KCounterTextEdit→QTextEdit+счётчик+maxLen.
//
// DEVICE в порт не тянется: ClickBtnSave (persist в БД), ClickBtnSaveAsWordBank (запись
// в глоссарий), UploadDicom/print/preview, синхронизация вьюх изображений, сериализация
// курсора, все источники данных комбо/дерева/картинок — заглушки. Exit→close.
// Счётчики символов и обрезка по maxLen — чистый UI, реализованы.
class KReportEditUi : public KDialog
{
    Q_OBJECT
public:
    explicit KReportEditUi(QWidget *parent = nullptr);

private:
    void setupUi();
};
