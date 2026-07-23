#pragma once

#include <QWidget>
#include <QImage>
#include <QString>

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

    // Реф. KViewSoftEndo::DisplayMsg(QString) — сообщение поверх кадра
    // (зовётся из KViewBase::ShowExportErrorMsg @0x45bdd0).
    void DisplayMsg(const QString &msg);
    QString LastMsg() const { return m_lastMsg; }

public slots:
    void OnVideoFrameReady(const QImage &frame); // приём кадра от KVideoProxy

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QImage  frame_;
    QString m_lastMsg;
};
