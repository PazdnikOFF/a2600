#include "video/KSaveVideoFile.h"

#include <QDir>
#include <QFileInfo>
#include <QStorageInfo>
#include <QDebug>

KSaveVideoFile::KSaveVideoFile(QObject *parent) : QObject(parent)
{
    if (!gst_is_initialized())
        gst_init(nullptr, nullptr);
}

KSaveVideoFile::~KSaveVideoFile()
{
    StopRecord();
}

bool KSaveVideoFile::StartRecord(const QString &mp4Path, const QString &device,
                                 int width, int height)
{
    StopRecord();
    QDir().mkpath(QFileInfo(mp4Path).absolutePath());

    // VCU-энкодер (из реверса): NV12 → omxh264enc → h264parse → mp4mux → filesink.
    const QString desc = QString(
        "v4l2src device=%1 io-mode=dmabuf ! "
        "video/x-raw,format=NV12,width=%2,height=%3 ! "
        "omxh264enc ! video/x-h264,profile=high ! h264parse ! "
        "mp4mux ! filesink location=%4")
        .arg(device).arg(width).arg(height).arg(mp4Path);

    GError *err = nullptr;
    pipeline_ = gst_parse_launch(desc.toUtf8().constData(), &err);
    if (!pipeline_ || err) {
        if (err) { qWarning() << "KSaveVideoFile:" << err->message; g_error_free(err); }
        StopRecord();
        return false;
    }
    if (gst_element_set_state(pipeline_, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        StopRecord();
        return false;
    }
    qInfo() << "KSaveVideoFile::StartRecord:" << desc;
    return true;
}

void KSaveVideoFile::StopRecord()
{
    if (!pipeline_)
        return;
    // Корректно закрыть mp4: послать EOS и дождаться, затем NULL.
    gst_element_send_event(pipeline_, gst_event_new_eos());
    GstBus *bus = gst_element_get_bus(pipeline_);
    gst_bus_timed_pop_filtered(bus, 3 * GST_SECOND,
                               GstMessageType(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    gst_object_unref(bus);
    gst_element_set_state(pipeline_, GST_STATE_NULL);
    gst_object_unref(pipeline_);
    pipeline_ = nullptr;
}

QImage KSaveVideoFile::CreateVideoSmallImage(const QImage &frame, const QSize &cell)
{
    // Реф. CreateVideoSmallImage — миниатюра под ячейку списка снимков.
    if (frame.isNull())
        return QImage();
    return frame.scaled(cell, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

bool KSaveVideoFile::SaveVideoThumbnail(const QImage &frame, const QString &path,
                                        const QSize &cell)
{
    const QImage thumb = CreateVideoSmallImage(frame, cell);
    if (thumb.isNull())
        return false;
    QDir().mkpath(QFileInfo(path).absolutePath());
    return thumb.save(path, "JPEG", 85);
}

bool KSaveVideoFile::CheckIsToLimitStatus(const QString &path, qint64 minFreeBytes)
{
    // Реф. CheckIsToLimitStatus — достигнут ли предел свободного места.
    QStorageInfo si(path);
    return si.isValid() && si.bytesAvailable() < minFreeBytes;
}
