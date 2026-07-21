#include "KProgressDlg.h"

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

    QLabel *lblText = new QLabel(tr("TR_Prpng"), frame);   // статус/«подготовка»
    lblText->setObjectName(QStringLiteral("label_Text"));
    lblText->setMaximumHeight(50);
    lblText->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
    v->addWidget(lblText);

    // Блок из двух прогресс-баров.
    QVBoxLayout *v2 = new QVBoxLayout();
    v2->setObjectName(QStringLiteral("verticalLayout_2"));
    QLabel *lblCur = new QLabel(tr("TR_CFProgress"), frame);
    lblCur->setObjectName(QStringLiteral("lblCurrentProgress")); lblCur->setMinimumWidth(40);
    v2->addWidget(lblCur);
    QProgressBar *progCur = new QProgressBar(frame);
    progCur->setObjectName(QStringLiteral("progCurrentFile"));
    progCur->setValue(0);
    progCur->setStyleSheet(QStringLiteral("QProgressBar{border:2px solid gray;}"));   // реф. Init
    v2->addWidget(progCur);
    QLabel *lblTot = new QLabel(tr("TR_TProgress"), frame);
    lblTot->setObjectName(QStringLiteral("lblTotalProgress")); lblTot->setMinimumWidth(40);
    v2->addWidget(lblTot);
    QProgressBar *progTot = new QProgressBar(frame);
    progTot->setObjectName(QStringLiteral("progTotal"));
    progTot->setValue(0);
    progTot->setStyleSheet(QStringLiteral("QProgressBar{border:2px solid gray;}"));
    v2->addWidget(progTot);
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
    // ToDlgMsgDispatcher-сигналы (Set*Progress/Label) — DEVICE, не подключаем.
}
