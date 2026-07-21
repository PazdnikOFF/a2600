#include "KTimeWasteBar.h"

#include <QLabel>
#include <QProgressBar>
#include <QGridLayout>
#include <QSpacerItem>
#include <QKeyEvent>

KTimeWasteBar::KTimeWasteBar(QWidget *parent, int nSeconds)
    : KDialog(parent, false)
{
    // Реф. ctor @0x6095b0: m_nMax=nSeconds*10 (или 100), resize(480,160), SetKStyle(4)=W480,
    // title TR_Dlg, grid(spacer/label/progbar/spacer), QTimer 100мс → TimeSlot, старт.
    m_nMax = nSeconds > 0 ? nSeconds * 10 : 100;
    setObjectName(QStringLiteral("KTimeWasteBar"));
    resize(480, 160);
    SetKStyle(KDLG_W480);
    SetTitle(tr("TR_Dlg"));

    QWidget *content = ContentArea();
    QGridLayout *g = new QGridLayout(content);
    g->setObjectName(QStringLiteral("gridLayout"));
    g->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding), 0, 0);

    m_label = new QLabel(content);
    m_label->setObjectName(QStringLiteral("label_Text"));
    m_label->setMaximumSize(16777215, 50);
    m_label->setTextFormat(Qt::PlainText);
    m_label->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
    m_label->setText(tr("TR_PWait"));
    g->addWidget(m_label, 1, 0);

    m_progBar = new QProgressBar(content);
    m_progBar->setObjectName(QStringLiteral("ProgBar"));
    m_progBar->setValue(0);   // реф.: старт с 0 (после начального 24)
    g->addWidget(m_progBar, 2, 0);

    g->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding), 3, 0);

    connect(&m_timer, &QTimer::timeout, this, &KTimeWasteBar::TimeSlot);
    m_timer.start(100);   // реф.: 100мс, старт безусловно
}

void KTimeWasteBar::TimeSlot()
{
    // Реф. @0x609538: ++count, pct=count*100/max, ≥100→close, setValue(pct).
    ++m_nCount;
    const int pct = int(m_nCount * 100.0 / m_nMax);
    if (pct >= 100)
        close();
    m_progBar->setValue(pct);
}

void KTimeWasteBar::keyPressEvent(QKeyEvent *e)
{
    // Реф. @0x609518: Esc → close ТОЛЬКО если m_bEscEnable; иначе съесть.
    if (e->key() == Qt::Key_Escape) {
        if (m_bEscEnable)
            close();
        return;   // съесть (реф. не зовёт базу)
    }
    KDialog::keyPressEvent(e);
}
