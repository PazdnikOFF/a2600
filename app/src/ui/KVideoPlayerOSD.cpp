#include "KVideoPlayerOSD.h"
#include "KImageButton.h"
#include "Theme.h"

#include <QDir>
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
    // АПГРЕЙД: реальные KImageButton (был QPushButton с текстом). Реверс: 8 transport-кнопок =
    // KImageButton, LoadStyleSheet зовёт SetImage(normal, hover=selected, selected, invalid).
    // Реф. base = "02-data/rw-common/picture" (device-путь); локально ассеты в qss/black/player/btn.
    const QString btnBase = QDir(theme::root()).absoluteFilePath(QStringLiteral("qss/black/player/btn"));
    auto btn = [&](const char *name, const QString &normal, const QString &selected,
                   const QString &invalid) {
        KImageButton *b = new KImageButton(widget);
        b->setObjectName(QString::fromLatin1(name));
        b->SetCustomBasePath(btnBase);
        b->SetImage(normal, selected, selected, invalid);   // реф. SetImage(n, hover=sel, sel, inv)
        b->SetSize(75, 46);
        b->setFocusPolicy(Qt::NoFocus);
        h->addWidget(b);
        return b;
    };
    btn("btn_last_frame", QStringLiteral("last_frame_normal.png"),
        QStringLiteral("last_frame_selected.png"), QStringLiteral("last_frame_disable.png"));
    // реф. КВИРК: у next_frame invalid-иконка = next_video_disable.png (баг реф., воспроизводим).
    btn("btn_next_frame", QStringLiteral("next_frame_normal.png"),
        QStringLiteral("next_frame_selected.png"), QStringLiteral("next_video_disable.png"));
    btn("btn_last_video", QStringLiteral("last_video_normal.png"),
        QStringLiteral("last_video_selected.png"), QStringLiteral("last_video_disable.png"));
    // реф. start_pause invalid = pause_normal.png (pause_disable.png не существует).
    btn("btn_start_pause", QStringLiteral("pause_normal.png"),
        QStringLiteral("pause_selected.png"), QStringLiteral("pause_normal.png"));
    btn("btn_next_video", QStringLiteral("next_video_normal.png"),
        QStringLiteral("next_video_selected.png"), QStringLiteral("next_video_disable.png"));
    // btn_speed — БЕЗ иконки, runtime-текст "X1.0" (реф.).
    KImageButton *btnSpeed = new KImageButton(widget);
    btnSpeed->setObjectName(QStringLiteral("btn_speed"));
    btnSpeed->setText(QStringLiteral("X1.0"));
    btnSpeed->SetSize(75, 46);
    btnSpeed->setFocusPolicy(Qt::NoFocus);
    h->addWidget(btnSpeed);
    btn("btn_save_frame", QStringLiteral("save_normal.png"),
        QStringLiteral("save_selected.png"), QStringLiteral("save_disable.png"));
    KImageButton *btnExit = btn("btn_exit", QStringLiteral("exit_normal.png"),
        QStringLiteral("exit_selected.png"), QStringLiteral("exit_disable.png"));
    v2->addLayout(h);
    root->addWidget(widget);

    connect(btnExit, &QPushButton::clicked, this, &QWidget::close);   // реф. OnBtnExitClicked→close
    // Play/seek/speed/save + KVideoPlayer/QTimer — DEVICE, не подключаем.
}
