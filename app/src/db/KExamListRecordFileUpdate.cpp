#include "db/KExamListRecordFileUpdate.h"

#include <QDir>
#include <QFileInfoList>

int KExamListRecordFileUpdate::GetFiletypeNumFromPath(const QString &path,
                                                      const QStringList &exts)
{
    if (path.isEmpty())
        return 0;
    QDir dir(path);
    if (!dir.exists())
        return 0;
    // Реф.: setNameFilters(exts) → entryInfoList(Files) → размер списка.
    return dir.entryInfoList(exts, QDir::Files).size();
}

int KExamListRecordFileUpdate::ImageFileNum(const QString &dir)
{
    static const QStringList kImg{"*.jpg", "*.bmp", "*.png", "*.jpeg"};
    return GetFiletypeNumFromPath(dir, kImg);
}

int KExamListRecordFileUpdate::VideoFileNum(const QString &dir)
{
    static const QStringList kVid{"*.mp4", "*.avi"};
    return GetFiletypeNumFromPath(dir, kVid);
}
