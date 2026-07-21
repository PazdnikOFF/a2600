#include "KVideoPlayerOSD.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

KVideoPlayerOSD::KVideoPlayerOSD(QWidget *parent)
    : QWidget(parent)
{
    // Реф. ctor @0x3d47d8: QDialog(parent,0) → KVideoPlayer + 2 QTimer → setupUi →
    // frameless always-on-top → setFixedSize(GetUIResolution) → slider range. Портируем как QWidget.
    setupUi();
}

void KVideoPlayerOSD::setupUi()
{
    setObjectName(QStringLiteral("KVideoPlayerOSD"));
    setMinimumWidth(1280);
    setStyleSheet(QStringLiteral(
        "KVideoPlayerOSD{background:rgb(20,21,25);}"
        "QLabel{font-size:20px;font-weight:800;color:white;}"
        "QPushButton{border:none;font-weight:1000;color:white;}"
        "QSlider::groove:horizontal{background-color:rgb(1,188,196);height:8px;}"
        "QSlider::add-page:horizontal{background-color:rgb(84,88,88);}"
        "QSlider::handle:horizontal{background:white;width:22px;height:22px;margin:-11px 0;}"));

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 6, 0, 6);

    // Имя файла (реф. placeholder, device перезаписывает).
    QLabel *lblFile = new QLabel(QStringLiteral("202405210003.mp4"), this);
    lblFile->setObjectName(QStringLiteral("label_file_name"));
    root->addWidget(lblFile);

    QWidget *widget = new QWidget(this);
    widget->setObjectName(QStringLiteral("widget"));
    QVBoxLayout *v2 = new QVBoxLayout(widget);
    v2->setContentsMargins(0, 0, 0, 0);

    // Seek-слайдер.
    QSlider *slider = new QSlider(Qt::Horizontal, widget);
    slider->setObjectName(QStringLiteral("horizontalSlider"));
    slider->setRange(0, 100);
    v2->addWidget(slider);

    // Ряд: время + transport-кнопки.
    QHBoxLayout *h = new QHBoxLayout();
    h->setObjectName(QStringLiteral("horizontalLayout"));
    h->setContentsMargins(12, 0, 24, 0);
    QHBoxLayout *h2 = new QHBoxLayout();
    QLabel *lblCur = new QLabel(QStringLiteral("00:19:23"), widget);
    lblCur->setObjectName(QStringLiteral("label_cur_time"));
    h2->addWidget(lblCur);
    QLabel *lblTot = new QLabel(QStringLiteral("/02:01:56"), widget);
    lblTot->setObjectName(QStringLiteral("label_total_time"));
    h2->addWidget(lblTot);
    h->addLayout(h2);
    h->addStretch(1);
    auto btn = [&](const char *name, const QString &text) {   // реф. KImageButton → QPushButton
        QPushButton *b = new QPushButton(text, widget);
        b->setObjectName(QString::fromLatin1(name));
        b->setMinimumWidth(75);
        b->setFocusPolicy(Qt::NoFocus);
        h->addWidget(b);
        return b;
    };
    btn("btn_last_frame", QStringLiteral("LastFrame"));   // реф. seek −1 кадр (device)
    btn("btn_next_frame", QStringLiteral("NextFrame"));
    btn("btn_last_video", QStringLiteral("LastVideo"));
    btn("btn_start_pause", QStringLiteral("Play/Pause"));
    btn("btn_next_video", QStringLiteral("NextVideo"));
    btn("btn_speed", QStringLiteral("Speed"));
    btn("btn_save_frame", QStringLiteral("Save"));
    QPushButton *btnExit = btn("btn_exit", QStringLiteral("Exit"));
    v2->addLayout(h);
    root->addWidget(widget);

    connect(btnExit, &QPushButton::clicked, this, &QWidget::close);   // реф. OnBtnExitClicked→close
    // Play/seek/speed/save + KVideoPlayer/QTimer — DEVICE, не подключаем.
}
