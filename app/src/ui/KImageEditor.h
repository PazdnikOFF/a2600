#pragma once

#include "ui/KDialog.h"

// Диалог аннотирования снятого изображения (реф. KImageEditor : KFullScreenDialog — НЕ
// KDialog!, ctor @0x4a1cc8, Ui_KImageEditor::setupUi @0x4a40d8). UI-порт. Из workflow
// захвата/просмотра: грузит один снимок в графический холст и позволяет ставить
// курсор-стрелку (Lock/LeftUp/LeftDown/RightUp/RightDown). Полноэкранный 1920×1080
// (реф. base KFullScreenDialog(parent,-1), 2-й арг = int-id -1, не bool) → портируем над
// KDialog(FULLSCREEN). SetKStyle НЕТ, титул SetTitle(TR_IProcessing).
// Слева graphicsView (KImageEditorGraphicsView→QGraphicsView, чёрный фон) + label_file_name
// (device — имя файла). Справа панель: label TR_CType + 5 icon-only radio (иконки —
// ресурсы, у нас глиф-замена; дефолт Lock) + спейсер + Prev(F4)/Next(F5)/Save(F1)/
// Delete(Del)/Exit(Esc).
//
// DEVICE в порт не тянется: SetArrow* (режим курсора графики), OnBtnSave/OnBtnDelete
// (запись аннотации в снимок), SetChangedWithoutSave (dirty-флаг), label_file_name —
// заглушки. Exit→close; Prev/Next — чистый UI-нав (no-op).
class KImageEditor : public KDialog
{
    Q_OBJECT
public:
    explicit KImageEditor(QWidget *parent = nullptr);

private:
    void setupUi();
};
