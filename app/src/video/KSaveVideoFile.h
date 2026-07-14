#pragma once

#include <QObject>
#include <QString>
#include <QImage>
#include <gst/gst.h>

// Запись видео в файл (реф. класс KSaveVideoFile, X-2600).
// Реальные методы: Save, CreateVideoSmallImage/BigImage (миниатюры),
// SaveVideoThumbnail, CheckIsToLimitStatus/CheckIsToTipStatue (контроль хранилища).
//
// Формат записи (из реверса): MP4/H.264 через VCU: omxh264enc → h264parse →
// mp4mux → filesink. Имя файла — timestamp .mp4. Миниатюра — 190×135 (ячейка
// KImgList). В оригинале ветка энкодера ответвляется (tee) от живого тракта;
// здесь (реимплементация) отдельный V4L2-читатель того же устройства.
class KSaveVideoFile : public QObject
{
    Q_OBJECT
public:
    explicit KSaveVideoFile(QObject *parent = nullptr);
    ~KSaveVideoFile() override;

    bool StartRecord(const QString &mp4Path, const QString &device,
                     int width, int height);
    void StopRecord();
    bool IsRecording() const { return pipeline_ != nullptr; }

    // Миниатюра кадра (реф. CreateVideoSmallImage) — для списка снимков.
    static QImage CreateVideoSmallImage(const QImage &frame, const QSize &cell);
    static bool   SaveVideoThumbnail(const QImage &frame, const QString &path,
                                     const QSize &cell);

    // Контроль лимита хранилища (реф. CheckIsToLimitStatus/CheckIsToTipStatue).
    static bool CheckIsToLimitStatus(const QString &path, qint64 minFreeBytes);

private:
    GstElement *pipeline_ = nullptr;
};
