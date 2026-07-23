#include "KBackGround.h"

#include <QTimer>

KBackGround::KBackGround(QWidget *parent)
    : KDialog(parent, false)
{
    // Реф. ctor @0x457cd8, порядок сохранён.
    setObjectName(QStringLiteral("KBackGround"));
    resize(400, 300);
    setWindowTitle(tr("TR_Dlg"));
    SetKStyle(KDLG_FULLSCREEN);      // реф. SetKStyle(1)
    SetBtnCloseVisible(false);       // реф. SetBtnCloseVisble(false)

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &KBackGround::close);
    m_timer->start(1000);            // реф. интервал 1000 мс
}

void OpenBackGround()
{
    // Реф. @0x457fe8: new → exec → delete. В прошивке НИКЕМ не вызывается.
    KBackGround dlg;
    dlg.exec();
}
