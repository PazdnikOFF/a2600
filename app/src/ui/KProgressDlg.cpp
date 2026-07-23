#include "KProgressDlg.h"
#include "KToProgressDlgMsgDispatcher.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

KProgressDlg::KProgressDlg(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x451870: QDialog(parent,0) → setupUi → setStyleSheet(рамка/font18 +
    // border прогресс-баров) → InitConnect. Портируем над KDialog.
    setupUi();
    SetTitle(tr("TR_Prpng"));
}

void KProgressDlg::setupUi()
{
    setObjectName(QStringLiteral("KProgressDlg"));
    resize(512, 278);

    QWidget *host = ContentArea();
    host->setStyleSheet(QStringLiteral("*{font-size:18px;}"));   // реф. общий стиль
    QVBoxLayout *rootL = new QVBoxLayout(host);
    rootL->setContentsMargins(0, 0, 0, 0);

    // Фрейм-контейнер (реф. umessage_frame_back).
    QFrame *frame = new QFrame(host);
    frame->setObjectName(QStringLiteral("umessage_frame_back"));
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setFrameShadow(QFrame::Raised);
    frame->setLineWidth(1);
    QVBoxLayout *v = new QVBoxLayout(frame);
    v->setObjectName(QStringLiteral("verticalLayout"));

    m_lblText = new QLabel(tr("TR_Prpng"), frame);   // статус/«подготовка»
    m_lblText->setObjectName(QStringLiteral("label_Text"));
    m_lblText->setMaximumHeight(50);
    m_lblText->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
    v->addWidget(m_lblText);

    // Блок из двух прогресс-баров.
    QVBoxLayout *v2 = new QVBoxLayout();
    v2->setObjectName(QStringLiteral("verticalLayout_2"));
    m_lblCurrent = new QLabel(tr("TR_CFProgress"), frame);
    m_lblCurrent->setObjectName(QStringLiteral("lblCurrentProgress")); m_lblCurrent->setMinimumWidth(40);
    v2->addWidget(m_lblCurrent);
    m_progCurrent = new QProgressBar(frame);
    m_progCurrent->setObjectName(QStringLiteral("progCurrentFile"));
    m_progCurrent->setValue(0);
    m_progCurrent->setStyleSheet(QStringLiteral("QProgressBar{border:2px solid gray;}"));   // реф. Init
    v2->addWidget(m_progCurrent);
    m_lblTotal = new QLabel(tr("TR_TProgress"), frame);
    m_lblTotal->setObjectName(QStringLiteral("lblTotalProgress")); m_lblTotal->setMinimumWidth(40);
    v2->addWidget(m_lblTotal);
    m_progTotal = new QProgressBar(frame);
    m_progTotal->setObjectName(QStringLiteral("progTotal"));
    m_progTotal->setValue(0);
    m_progTotal->setStyleSheet(QStringLiteral("QProgressBar{border:2px solid gray;}"));
    v2->addWidget(m_progTotal);
    v->addLayout(v2);

    v->addStretch(1);   // реф. вертикальный спейсер

    // Ряд Cancel (центрирован).
    QHBoxLayout *h = new QHBoxLayout();
    h->setObjectName(QStringLiteral("horizontalLayout"));
    h->addStretch(1);
    QPushButton *btnCancel = new QPushButton(tr("TR_Ccl"), frame);
    btnCancel->setObjectName(QStringLiteral("btn_Cancel"));
    btnCancel->setMinimumWidth(150);
    h->addWidget(btnCancel);
    h->addStretch(1);
    v->addLayout(h);

    rootL->addWidget(frame);

    connect(btnCancel, &QPushButton::clicked, this, &QWidget::close);   // реф. OnCancel→close
    InitConnect();
}

void KProgressDlg::InitConnect()
{
    // Реф. @0x451320: PMF-connect ВСЕХ восьми сигналов ToDlgMsgDispatcher() 1:1.
    auto *d = ToDlgMsgDispatcher();
    connect(d, &KToProgressDlgMsgDispatcher::SigUpdateTitleTotalProgress,
            this, &KProgressDlg::OnSigUpdateTitleTotalProgress);
    connect(d, &KToProgressDlgMsgDispatcher::SigUpdateHide, this, &KProgressDlg::OnSigUpdateHide);
    connect(d, &KToProgressDlgMsgDispatcher::SigShowResultMsgBox,
            this, &KProgressDlg::OnSigShowResultMsgBox);
    connect(d, &KToProgressDlgMsgDispatcher::SigUpdateTotalProgress,
            this, &KProgressDlg::OnSigUpdateTotalProgress);
    connect(d, &KToProgressDlgMsgDispatcher::SigUpdateSubProgress,
            this, &KProgressDlg::OnSigUpdateSubProgress);
    connect(d, &KToProgressDlgMsgDispatcher::SigUpdateTotalLabel,
            this, &KProgressDlg::OnSigUpdateTotalLabel);
    connect(d, &KToProgressDlgMsgDispatcher::SigUpdateSubLabel,
            this, &KProgressDlg::OnSigUpdateSubLabel);
    connect(d, &KToProgressDlgMsgDispatcher::SigOneExamRecordUpdateFinish,
            this, &KProgressDlg::OnSigOneExamRecordUpdateFinish);
}

void KProgressDlg::OnSigUpdateTitleTotalProgress(const QString &title, int totalProgress)
{
    SetTitle(title);
    m_progTotal->setValue(totalProgress);
}
void KProgressDlg::OnSigUpdateHide()                     { hide(); }
void KProgressDlg::OnSigShowResultMsgBox(const QString &msg)
{
    // Реф. показывает KMessageBox с результатом — у нас текст уходит в статус-метку
    // (сам месседж-бокс поднимает вызывающий).
    m_lblText->setText(msg);
}
void KProgressDlg::OnSigUpdateTotalProgress(int p)       { m_progTotal->setValue(p); }
void KProgressDlg::OnSigUpdateSubProgress(int p)         { m_progCurrent->setValue(p); }
void KProgressDlg::OnSigUpdateTotalLabel(const QString &t) { m_lblTotal->setText(t); }
void KProgressDlg::OnSigUpdateSubLabel(const QString &t)   { m_lblCurrent->setText(t); }
void KProgressDlg::OnSigOneExamRecordUpdateFinish(const KMessage &msg)
{
    // Реф.: закрытие/обновление по завершении записи осмотра — device-логика; в порте
    // фиксируем факт (сообщение помечено обработанным вызывающим).
    Q_UNUSED(msg);
    m_progTotal->setValue(m_progTotal->maximum());
}
