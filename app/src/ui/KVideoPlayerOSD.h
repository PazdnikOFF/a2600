#pragma once

#include <QWidget>

// OSD-панель управления видеоплеером (реф. KVideoPlayerOSD : QDialog — frameless always-on-top
// оверлей, ctor @0x3d47d8, Ui_KVideoPlayerOSD::setupUi @0x3d4b20). UI-порт. Реф. — полноэкранный
// безрамочный оверлей поверх видео; порт как самостоятельный QWidget (панель управления).
// vbox: label_file_name + [horizontalSlider (styled QSS: groove rgb1,188,196 / add-page
// rgb84,88,88 / handle-image slider_inspecto.png) + hbox[время cur/total + 8 transport-кнопок
// LastFrame/NextFrame/LastVideo/Play-Pause/NextVideo/Speed/Save/Exit]]. Тексты — англ.
// placeholder-литералы (НЕ TR; device перезаписывает имя/время). Кастом KImageButton→QPushButton.
//
// DEVICE в порт не тянется: KVideoPlayer (движок декода/воспроизведения), 2 QTimer (refresh/
// mouse-hide), GetUIResolution (fixed-size), transport-слоты (Play/seek/speed/save) — заглушки.
// btn_exit→close.
class KVideoPlayerOSD : public QWidget
{
    Q_OBJECT
public:
    explicit KVideoPlayerOSD(QWidget *parent = nullptr);

private:
    void setupUi();
};
