#include "video/KViewSoftEndo.h"

#include <QLabel>
#include <QPainter>

KViewSoftEndo::KViewSoftEndo(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::black);
    setPalette(pal);
    setAutoFillBackground(true);

    // Оверлей системного времени (реф. label_Systemtime, QRect(10,9,171,51)).
    systemTimeLabel_ = new QLabel(this);
    systemTimeLabel_->setObjectName(QStringLiteral("label_Systemtime"));
    systemTimeLabel_->setGeometry(10, 9, 171, 51);
}

void KViewSoftEndo::UpdateSystemtime(const QString &text)
{
    if (systemTimeLabel_)
        systemTimeLabel_->setText(text);   // реф. @0x4672e0
}

void KViewSoftEndo::InitVideoParam() { /* инициализация OSD-параметров вида */ }
void KViewSoftEndo::InitStatus()     { frame_ = QImage(); update(); }
void KViewSoftEndo::FreezeAck()      { /* индикация стоп-кадра */ }
void KViewSoftEndo::ImageSaveAck()   { /* индикация сохранения снимка */ }

void KViewSoftEndo::OnVideoFrameReady(const QImage &frame)
{
    frame_ = frame;
    update();
}

void KViewSoftEndo::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), Qt::black);
    if (frame_.isNull())
        return;

    const QSize scaled = frame_.size().scaled(size(), Qt::KeepAspectRatio);
    const QRect target(QPoint((width() - scaled.width()) / 2,
                              (height() - scaled.height()) / 2), scaled);
    p.drawImage(target, frame_);
}

void KViewSoftEndo::DisplayMsg(const QString &msg)
{
    m_lastMsg = msg;   // реф. пишет поверх кадра (device-OSD)
}
