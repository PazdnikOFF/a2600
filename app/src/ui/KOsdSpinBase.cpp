#include "KOsdSpinBase.h"

KOsdSpinBase::KOsdSpinBase(QWidget *parent)
    : KFrame(parent)
{
    // Реф. ctor @0x485570: KFrame(parent) + m_gotFocusIn = false.
}

void KOsdSpinBase::ConfirmKeyAct(int index)
{
    // Реф. @0x4855b0: тумблер захвата фокуса, СНАЧАЛА визуал, потом хук подкласса.
    m_gotFocusIn = !m_gotFocusIn;
    if (m_gotFocusIn)
        Focused();
    else
        Selected();
    ConfirmKeyActImpl(index);
}

void KOsdSpinBase::UpKeyAct()
{
    // Реф. @0x485628: без захвата фокуса Up игнорируется (курсор двигает меню).
    if (GotFocusIn())
        AddValue();
}

void KOsdSpinBase::DownKeyAct()
{
    // Реф. @0x485668.
    if (GotFocusIn())
        SubValue();
}
