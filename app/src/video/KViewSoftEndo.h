#pragma once

#include <QWidget>
#include <QImage>

// Вьювер живого видео гибкого эндоскопа. Имя и методы — из оригинального
// KViewSoftEndo (реф. X-2600): InitVideoParam, InitStatus, FreezeAck,
// ImageSaveAck, ClickColorBMode/CMode/RMode (ENH/COLOR/RBC).
//
// Размещается в области IMAGE основного монитора (Rect из display.ini).
// При отсутствии эндоскопа область чёрная.
class KViewSoftEndo : public QWidget
{
    Q_OBJECT
public:
    explicit KViewSoftEndo(QWidget *parent = nullptr);

    void InitVideoParam();   // инициализация параметров вида
    void InitStatus();       // сброс статуса/OSD
    void FreezeAck();        // подтверждение стоп-кадра
    void ImageSaveAck();     // подтверждение сохранения снимка

public slots:
    void OnVideoFrameReady(const QImage &frame); // приём кадра от KVideoProxy

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QImage frame_;
};
