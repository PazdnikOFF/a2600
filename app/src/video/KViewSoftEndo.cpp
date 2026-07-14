#include "video/KViewSoftEndo.h"

#include <QPainter>

KViewSoftEndo::KViewSoftEndo(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::black);
    setPalette(pal);
    setAutoFillBackground(true);
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
