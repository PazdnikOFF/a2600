#pragma once

#include <QObject>
#include <QString>

// Файловый утилити-слой резервирования/экспорта (реф. KFileBackup, X-2600).
// Рекурсивное копирование/удаление каталогов, размер, свободное место, тип
// целевого устройства (внутр./USB) по пути. Используется бэкапом/экспортом
// осмотров на USB. QObject: сигнал прогресса FileBackupProgress.
class KFileBackup : public QObject
{
    Q_OBJECT
public:
    enum DeviceType { DevUnknown = 0, DevInternal, DevUsb };

    explicit KFileBackup(QObject *parent = nullptr) : QObject(parent) {}

    // Рекурсивно скопировать каталог (реф. copyDirectoryFiles). includeSub —
    // заходить в подкаталоги. Возвращает успех.
    bool copyDirectoryFiles(const QString &src, const QString &dst, bool includeSub = true);
    bool copyFile(const QString &src, const QString &dst);          // реф. copyFile/FileCopy
    // Реф. сигнатура (0x61f8e0): int copyFile(QString, QString, bool overwrite),
    // где 1 — успех, отрицательные (-1..-5) — различные ошибки. Именно её
    // сравнивает с 1 KExportRecord::ExportFiles.
    int  copyFile(QString src, QString dst, bool overwrite);
    // Рекурсивно удалить каталог с содержимым (реф. removeDirWithContent).
    bool removeDirWithContent(const QString &dir);
    bool clearFolder(const QString &dir);                           // очистить (оставить сам каталог)

    // Суммарный размер файлов каталога, байт (реф. getFilesSize).
    qint64 getFilesSize(const QString &dir) const;
    // Свободное место на устройстве пути, байт (реф. getDiskFreeSpace).
    qint64 getDiskFreeSpace(const QString &path) const;
    // Тип устройства по целевому пути (реф. GetDeviceTypeFromTargetPath).
    DeviceType GetDeviceTypeFromTargetPath(const QString &path) const;

signals:
    void FileBackupProgress(int percent);   // реф. FileBackupProgress
};
