#include "KViewHardEndo.h"

#include <QGridLayout>
#include <QLabel>

#include "ui/KRigidEndoBtnGuide.h"

KViewHardEndo::KViewHardEndo(QWidget *parent)
    : QFrame(parent)
{
    // Реф. ctor @0x464a70: QFrame(parent,0) → setupUi → reparent оверлеев →
    // new KRigidEndoBtnGuide → InitUiConfig/InitShareWidget/InitVideoParam (device) →
    // UpdateUsbStatus/UpdateEndoDisconnectImage → eventFilter.
    setupUi();
}

void KViewHardEndo::setupUi()
{
    setObjectName(QStringLiteral("KViewHardEndo"));
    resize(891, 596);

    // Область видео (реф. label_video) — чёрная подложка.
    QLabel *video = new QLabel(this);
    video->setObjectName(QStringLiteral("label_video"));
    video->setGeometry(0, 0, 891, 596);
    video->setStyleSheet(QStringLiteral("background:rgb(1,1,1);"));

    auto mkFrame = [&](const char *name, int x, int y, int w, int h) {
        QFrame *f = new QFrame(this);
        f->setObjectName(QString::fromLatin1(name));
        f->setFrameShape(QFrame::StyledPanel); f->setFrameShadow(QFrame::Raised);
        f->setGeometry(x, y, w, h);
        return f;
    };

    // frame_time (реф. QRect(700,0,190,141)): rec-время/иконка/системное время.
    QFrame *frameTime = mkFrame("frame_time", 700, 0, 190, 141);
    QLabel *lblRec = new QLabel(QStringLiteral("00:00:00"), frameTime);   // реф. литерал (device)
    lblRec->setObjectName(QStringLiteral("label_rec")); lblRec->setGeometry(50, 10, 100, 24);
    QLabel *lblRecIcon = new QLabel(frameTime); lblRecIcon->setObjectName(QStringLiteral("label_recicon"));
    lblRecIcon->setGeometry(15, 10, 24, 24);
    QLabel *lblSys = new QLabel(frameTime); lblSys->setObjectName(QStringLiteral("label_Systemtime"));
    lblSys->setGeometry(10, 50, 170, 24); lblSys->setAlignment(Qt::AlignCenter);   // device-часы

    // frame_connect (gridLayout_4): статус/USB. Позиция — реалистичная (device-repos).
    QFrame *frameConn = mkFrame("frame_connect", 10, 10, 200, 60);
    QGridLayout *g4 = new QGridLayout(frameConn);
    QLabel *lblStat = new QLabel(frameConn); lblStat->setObjectName(QStringLiteral("label_status_space"));
    g4->addWidget(lblStat, 0, 0);
    QLabel *lblU1 = new QLabel(QStringLiteral("U1"), frameConn);   // реф. литерал
    lblU1->setObjectName(QStringLiteral("label_USB")); lblU1->setAlignment(Qt::AlignCenter);
    g4->addWidget(lblU1, 0, 1);
    QLabel *lblU2 = new QLabel(tr("U2"), frameConn);
    lblU2->setObjectName(QStringLiteral("label_USBSPace"));
    lblU2->setStyleSheet(QStringLiteral("font-size:14px;")); lblU2->setAlignment(Qt::AlignCenter);
    g4->addWidget(lblU2, 0, 2);

    // frame_lefttime (gridLayout_3): остаток времени записи.
    QFrame *frameLeft = mkFrame("frame_lefttime", 10, 80, 200, 80);
    QGridLayout *g3 = new QGridLayout(frameLeft);
    QLabel *lblLeftCap = new QLabel(tr("TR_RTime"), frameLeft);
    lblLeftCap->setObjectName(QStringLiteral("label_lefttime_cap"));
    lblLeftCap->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    g3->addWidget(lblLeftCap, 0, 0);
    QLabel *lblLeft = new QLabel(frameLeft); lblLeft->setObjectName(QStringLiteral("label_lefttime"));
    lblLeft->setStyleSheet(QStringLiteral("color:rgb(160,160,160);"));
    lblLeft->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    g3->addWidget(lblLeft, 1, 0);

    // frame_osd (хост OSD-меню, наполняется в runtime).
    mkFrame("frame_osd", 700, 160, 190, 300);

    // Гайд кнопок эндоскопа (реф. KRigidEndoBtnGuide, показывает реальный ассет).
    KRigidEndoBtnGuide *guide = new KRigidEndoBtnGuide(this);
    guide->setObjectName(QStringLiteral("widget_endobtnguide"));
    guide->setGeometry(175, 113, 540, 370);
}
