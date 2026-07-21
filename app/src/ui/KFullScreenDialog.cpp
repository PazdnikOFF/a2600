#include "KFullScreenDialog.h"

KFullScreenDialog::KFullScreenDialog(QWidget *parent, int id)
    : KDialog(parent, false)
    , KObject(id, nullptr)
{
    // Реф. ctor @0x3e95f0: KDialog + KObject(id) → SetKStyle(1)=FULLSCREEN.
    SetKStyle(KDLG_FULLSCREEN);
}
