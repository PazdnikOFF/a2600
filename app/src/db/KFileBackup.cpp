#include "db/KFileBackup.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStorageInfo>

bool KFileBackup::copyFile(const QString &src, const QString &dst)
{
    QDir().mkpath(QFileInfo(dst).absolutePath());
    if (QFile::exists(dst))
        QFile::remove(dst);
    return QFile::copy(src, dst);
}

int KFileBackup::copyFile(QString src, QString dst, bool overwrite)
{
    // Коды возврата реф.: 1 — успех; отрицательные — ошибки.
    // -1 нет исходного файла, -2 не создан каталог назначения,
    // -3 назначение существует и overwrite == false, -4 не удалось удалить
    // старый файл, -5 сбой копирования. (Точное распределение -1..-5 по
    // причинам в реф. восстановлено частично — помечено как допущение.)
    if (!QFile::exists(src))
        return -1;
    if (!QDir().mkpath(QFileInfo(dst).absolutePath()))
        return -2;
    if (QFile::exists(dst)) {
        if (!overwrite)
            return -3;
        if (!QFile::remove(dst))
            return -4;
    }
    return QFile::copy(src, dst) ? 1 : -5;
}

bool KFileBackup::copyDirectoryFiles(const QString &src, const QString &dst, bool includeSub)
{
    // реф.: QDir::mkpath dst; обход entryInfoList; файлы → copyFile, каталоги → рекурсия.
    QDir srcDir(src);
    if (!srcDir.exists())
        return false;
    if (!QDir().mkpath(dst))
        return false;
    const auto entries = srcDir.entryInfoList(
        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    bool ok = true;
    for (const QFileInfo &fi : entries) {
        const QString to = dst + "/" + fi.fileName();
        if (fi.isDir()) {
            if (includeSub)
                ok = copyDirectoryFiles(fi.filePath(), to, includeSub) && ok;
        } else {
            ok = copyFile(fi.filePath(), to) && ok;
        }
    }
    return ok;
}

bool KFileBackup::removeDirWithContent(const QString &dir)
{
    // реф.: рекурсивно удалить файлы и подкаталоги, затем сам каталог.
    QDir d(dir);
    if (!d.exists())
        return true;
    const auto entries = d.entryInfoList(
        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden);
    bool ok = true;
    for (const QFileInfo &fi : entries) {
        if (fi.isDir())
            ok = removeDirWithContent(fi.filePath()) && ok;
        else
            ok = QFile::remove(fi.filePath()) && ok;
    }
    return d.rmdir(dir.endsWith('/') ? dir.chopped(1) : dir) && ok;
}

bool KFileBackup::clearFolder(const QString &dir)
{
    QDir d(dir);
    if (!d.exists())
        return QDir().mkpath(dir);
    const auto entries = d.entryInfoList(
        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden);
    bool ok = true;
    for (const QFileInfo &fi : entries) {
        if (fi.isDir())
            ok = removeDirWithContent(fi.filePath()) && ok;
        else
            ok = QFile::remove(fi.filePath()) && ok;
    }
    return ok;
}

qint64 KFileBackup::getFilesSize(const QString &dir) const
{
    qint64 total = 0;
    QDir d(dir);
    const auto entries = d.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &fi : entries) {
        if (fi.isDir())
            total += getFilesSize(fi.filePath());
        else
            total += fi.size();
    }
    return total;
}

qint64 KFileBackup::getDiskFreeSpace(const QString &path) const
{
    QStorageInfo si(path);
    return si.isValid() ? si.bytesAvailable() : -1;
}

KFileBackup::DeviceType KFileBackup::GetDeviceTypeFromTargetPath(const QString &path) const
{
    // реф.: путь монтирования USB (/media, /mnt, /run/media) → USB, иначе внутренний.
    if (path.contains("/media") || path.contains("/mnt") || path.contains("/udisk") ||
        path.contains("/run/media"))
        return DevUsb;
    if (path.startsWith("/home") || path.startsWith("/data") || path.startsWith('/'))
        return DevInternal;
    return DevUnknown;
}
