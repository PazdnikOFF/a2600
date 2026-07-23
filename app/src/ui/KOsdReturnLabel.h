#pragma once

#include "KFrame.h"

class QHBoxLayout;
class QLabel;

// Строка «назад» в конце OSD-подменю (реф. KOsdReturnLabel : KFrame, ctor @0x482440
// БЕЗ параметров, sizeof 0x50). UI-порт.
//
// Раскладка (константы из ctor): фрейм 250×32 фикс, objectName "KOsdReturnLabel";
// QHBoxLayout "horizontalLayout" spacing 9, margins (9,0,0,0); единственный ребёнок —
// label_icon (QLabel, sizePolicy Preferred/Preferred). ⚠️ В ctor пиксмап НЕ ставится:
// стоит текст-плейсхолдер tr("icon"), картинка появляется только из Selected/UnSelected.
//
// Иконки (невиртуальные геттеры @0x4823d0/@0x482408): "return_normal.png" /
// "return_select.png" — qss/black/osd/ (KDisplayOption::GetOsdIconPixmap).
// Focused() НЕ переопределён: работает базовый KFrame (рамка #009EA7).
// ConfirmKeyAct(int) @0x4828b0 ИГНОРИРУЕТ аргумент и закрывает меню-владельца.
//
// Создаётся ровно в одном месте: KOsdSubMenu::AddAReturnBtnInTheEndIfNeeded() @0x47bf60
// (гейт по флагу ctor-а меню + защёлка «уже добавлена»).
class KOsdReturnLabel : public KFrame
{
    Q_OBJECT
public:
    explicit KOsdReturnLabel(QWidget *parent = nullptr);   // реф. ctor без parent (ставится в AddItem)

    static QString GetNormalImgPath();   // реф. @0x4823d0 → "return_normal.png"
    static QString GetSelectImgPath();   // реф. @0x482408 → "return_select.png"

    void Selected() override;      // реф. @0x4828d0: база + пиксмап select
    void UnSelected() override;    // реф. @0x4829a0: база + пиксмап normal
    void ConfirmKeyAct(int index) override;   // реф. @0x4828b0: close() меню-владельца

    QLabel *IconLabel() const { return m_labelIcon; }

private:
    QHBoxLayout *m_layout = nullptr;
    QLabel *m_labelIcon = nullptr;   // Ui[1]
};
