#include "KWaitProgressBar.h"

#include "ui/KProgressBar.h"

#include <QGridLayout>
#include <QKeyEvent>
#include <QWidget>

KWaitProgressBar::KWaitProgressBar(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x609bf8: WindowModal, grid c одним KProgressBar на всю клетку, центрирование
    // по KSystemSet::GetUIResolution() (device, опущено), connect(readToClose→close).
    setObjectName(QStringLiteral("KWaitProgressBar"));
    setWindowModality(Qt::WindowModal);
    resize(480, 195);   // реф. 480×160; +35 под титул KDialog
    SetTitle(tr("TR_Dlg"));

    QGridLayout *grid = new QGridLayout(ContentArea());
    grid->setObjectName(QStringLiteral("gridLayout"));
    grid->setContentsMargins(0, 0, 0, 0);

    m_progressbar = new KProgressBar(ContentArea());
    m_progressbar->setObjectName(QStringLiteral("progressbar"));
    grid->addWidget(m_progressbar, 0, 0);

    // Реф. connect(progressbar, readToClose(), this, close()) — сигнал KProgressBar; опущен
    // (KProgressBar — следующий реверс).
}

void KWaitProgressBar::keyPressEvent(QKeyEvent *event)
{
    // Реф. @0x60ae80: Esc НЕ закрывает диалог ожидания (глотается).
    if (event->key() == Qt::Key_Escape)
        return;
    KDialog::keyPressEvent(event);
}
