#include "KProgressBar.h"

#include "ui/Theme.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSpacerItem>
#include <QTimer>

KProgressBar::KProgressBar(QWidget *parent)
    : QWidget(parent)
{
    // Реф. ctor @0x604bc8: setupUi → загрузка 21 кадра спрайта → QTimer 200мс → connect'ы →
    // new OperateWaitThread (worker, device — опущен).
    setupUi();

    // Загрузка 21-кадрового спрайта крутилки из прошивочной темы (реф.
    // KDisplayOption::GetThemeQssPath("bar/progressbar/progressbar%1.png")).
    for (int i = 0; i < 21; ++i) {
        const QString rel = QStringLiteral("black/bar/progressbar/progressbar%1.png").arg(i);
        QPixmap pm(theme::asset(rel));
        m_frames.append(pm);
    }

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &KProgressBar::timerUpdate);
    connect(btn_Cancel, &QPushButton::clicked, this, &KProgressBar::OnCancel);
    timer->start(200);   // реф. интервал 200 мс

    // Показать первый кадр сразу.
    if (!m_frames.isEmpty() && !m_frames[0].isNull())
        label_icomprogress->setPixmap(m_frames[0]);
}

void KProgressBar::setupUi()
{
    // Реф. Ui_KProgressBar::setupUi @0x605090.
    setObjectName(QStringLiteral("KProgressBar"));
    setWindowTitle(tr("TR_Fm"));

    QGridLayout *g2 = new QGridLayout(this);
    g2->setObjectName(QStringLiteral("gridLayout_2"));
    g2->setContentsMargins(0, 0, 0, 0);

    QFrame *frame_back = new QFrame(this);
    frame_back->setObjectName(QStringLiteral("umessage_frame_back"));
    frame_back->setFrameShape(QFrame::StyledPanel);
    frame_back->setFrameShadow(QFrame::Raised);
    g2->addWidget(frame_back, 0, 0, 1, 1);

    QGridLayout *g = new QGridLayout(frame_back);
    g->setObjectName(QStringLiteral("gridLayout"));
    g->setContentsMargins(18, 6, 18, 6);

    label_Text = new QLabel(frame_back);
    label_Text->setObjectName(QStringLiteral("label_Text"));
    label_Text->setTextFormat(Qt::PlainText);
    label_Text->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
    label_Text->setText(tr("TR_Prpng"));   // "processing"
    g->addWidget(label_Text, 0, 0, 1, 3);

    label_icomprogress = new QLabel(frame_back);
    label_icomprogress->setObjectName(QStringLiteral("label_icomprogress"));
    label_icomprogress->setFixedSize(40, 40);   // крутилка
    label_icomprogress->setAlignment(Qt::AlignCenter);
    g->addWidget(label_icomprogress, 1, 1, 1, 1);

    ProgBar = new QProgressBar(frame_back);
    ProgBar->setObjectName(QStringLiteral("ProgBar"));
    ProgBar->setValue(0);
    ProgBar->setVisible(false);   // реф.: скрыт в ctor (виден только для процентных задач)
    g->addWidget(ProgBar, 2, 0, 1, 3);

    QHBoxLayout *hb = new QHBoxLayout();
    hb->setObjectName(QStringLiteral("horizontalLayout"));
    hb->addStretch(1);
    btn_Cancel = new QPushButton(frame_back);
    btn_Cancel->setObjectName(QStringLiteral("btn_Cancel"));
    btn_Cancel->setFixedWidth(120);
    btn_Cancel->setText(tr("TR_Ccl"));
    hb->addWidget(btn_Cancel);
    hb->addStretch(1);
    g->addLayout(hb, 4, 0, 1, 3);
}

void KProgressBar::timerUpdate()
{
    // Реф. @0x604ab8: worker-текст (device, опущен) + следующий кадр спрайта (frame % 21).
    if (m_frames.isEmpty())
        return;
    m_frame = (m_frame + 1) % m_frames.size();
    if (!m_frames[m_frame].isNull())
        label_icomprogress->setPixmap(m_frames[m_frame]);
}

void KProgressBar::OnCancel()
{
    // Реф. @0x6046a8: флаг отмены + смена текстов.
    label_Text->setText(tr("TR_Cclng."));   // "cancelling…"
    btn_Cancel->setText(tr("TR_Ext"));       // "Exit"
}

void KProgressBar::SetDisplayText(const QString &text)
{
    label_Text->setText(text);
}

void KProgressBar::SetProgressBarValue(int value)
{
    ProgBar->setVisible(true);
    ProgBar->setValue(value);
}

void KProgressBar::SetBtnCancelVisible(bool visible)
{
    btn_Cancel->setVisible(visible);
}

void KProgressBar::WaitStart()
{
    // Реф. @0x604a68: QThread::start(worker) — DEVICE. Здесь крутилка уже идёт по таймеру.
}
