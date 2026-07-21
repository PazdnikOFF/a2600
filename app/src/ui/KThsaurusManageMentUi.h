#pragma once

#include "ui/KDialog.h"

// Диалог управления глоссарием (реф. KThsaurusManageMentUi : KDialog — орфография
// «Thsaurus» сохранена как в бинарнике; ctor @0x4e75e8, Ui_KThsaurusManageMentUi::
// setupUi @0x4ea010). UI-порт. Немодальный, 1024×768, SetKStyle НЕТ, титул
// SetTitle(TR_Glry). Layout-based (НЕ абсолютная геометрия). Состав:
//   • верхняя панель редактирования: группа/заголовок (cmb_group editable + edit_title
//     maxLen100 + Grp:/Ttle: с «*») + находки/заключение (2 QTextEdit);
//   • нижняя панель обзора: cmb_device (5 статичных типов эндоскопа) + tree_model
//     (QTreeWidget 1 колонка) + edit_content (KTextEdit→QTextEdit, disabled);
//   • подсказка TR_RField + ряд кнопок Add/Edit/Clone/Delete/OK/Cancel (по 133px).
//
// Кастом KTextEdit→QTextEdit. Modify/Clone/Delete активны только при выделении в
// дереве (реф. RefreshBtnEnable) — чистый UI.
//
// DEVICE в порт не тянется: KThesaurusOpt (ReadFile/LoadGlryContent/UpdateTree →
// cmb_group+tree; Add/Modify/Clone/Del CRUD; SwitchDeviceType) — заглушки; дерево
// и группа пусты. OK/Cancel→close.
class KThsaurusManageMentUi : public KDialog
{
    Q_OBJECT
public:
    explicit KThsaurusManageMentUi(QWidget *parent = nullptr);

private:
    void setupUi();
};
