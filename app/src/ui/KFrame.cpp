#include "KFrame.h"

// Стили базы — дословно из .rodata (длины совпадают с `mov w1` в реф.).
// Квирк вендора сохранён: в «Focused» цвет записан как «rgb(0,  205,209)» — ДВА пробела.
static const char *kStyleFramed  = "QFrame{border: 2px solid #009EA7;} QLabel{border: 0px solid transparent; color: rgb(0,  205,209);}";   // @0x868a98 (98)
static const char *kStyleFramedS = "QFrame{border: 2px solid #009EA7;} QLabel{border: 0px solid transparent; color: rgb(221,221,221);}";   // @0x868b00 (98)
static const char *kStyleFramedG = "QFrame{border: 2px solid #009EA7;} QLabel{border: 0px solid transparent; color: rgb(100,100,100);}";   // @0x868a30 (98)
static const char *kStylePlain   = "QLabel{border: 0px solid transparent; color: rgb(221,221,221);}";   // @0x868ba8 (63)
static const char *kStylePlainG  = "QLabel{border: 0px solid transparent; color: rgb(100,100,100);}";   // @0x868b68 (63)

KFrame::KFrame(QWidget *parent)
    : QFrame(parent)
{
    // Реф. ctor @0x47f490: QFrame(parent,0), m_locatedMenu=null, m_greyed=m_selected=false.
}

void KFrame::Focused()
{
    // Реф. @0x47f580: серый вариант при IsGreyed(), иначе бирюзовый. m_selected НЕ трогается.
    setStyleSheet(QString::fromLatin1(IsGreyed() ? kStyleFramedG : kStyleFramed));
}

void KFrame::Selected()
{
    // Реф. @0x47f678: тот же серый при greyed; иначе рамка + светло-серый текст. m_selected=true.
    setStyleSheet(QString::fromLatin1(IsGreyed() ? kStyleFramedG : kStyleFramedS));
    m_selected = true;
}

void KFrame::UnSelected()
{
    // Реф. @0x47f780: рамки нет вообще (строка короче — 63 символа). m_selected=false.
    setStyleSheet(QString::fromLatin1(IsGreyed() ? kStylePlainG : kStylePlain));
    m_selected = false;
}

void KFrame::UpdateUI()
{
    // Реф. @0x47f888: ldrb [+0x41] → виртуальный вызов Selected()/UnSelected().
    if (m_selected)
        Selected();
    else
        UnSelected();
}
