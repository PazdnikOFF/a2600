#include "db/KDbFileOperation.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>

// statfs: на Linux (целевая платформа прибора) — <sys/vfs.h>, на macOS/BSD — <sys/mount.h>.
#ifdef __linux__
#include <sys/vfs.h>
#else
#include <sys/mount.h>
#endif

bool KDbFileOperation::IsFileExist(const std::string &path)
{
    return QFile::exists(QString::fromStdString(path));
}

bool KDbFileOperation::IsFileDirExist(const std::string &path)
{
    return QFileInfo::exists(QString::fromStdString(path));
}

long long KDbFileOperation::GetFileSize(const std::string &path)
{
    QFileInfo fi(QString::fromStdString(path));
    return fi.exists() ? fi.size() : -1;
}

bool KDbFileOperation::RemoveFile(const std::string &path)
{
    return QFile::remove(QString::fromStdString(path));
}

bool KDbFileOperation::CreateFolder(const std::string &path)
{
    return QDir().mkpath(QString::fromStdString(path));
}

bool KDbFileOperation::DeleteFolder(const std::string &path)
{
    return QDir(QString::fromStdString(path)).removeRecursively();
}

bool KDbFileOperation::CopyFileToFile(const std::string &src, const std::string &dst)
{
    return QFile::copy(QString::fromStdString(src), QString::fromStdString(dst));
}

void KDbFileOperation::GetFilesByFilter(const std::string &dir, const std::string &filter,
                                        std::vector<std::string> &out)
{
    QDir d(QString::fromStdString(dir));
    const QStringList entries = d.entryList(QStringList{QString::fromStdString(filter)},
                                            QDir::Files);
    for (const QString &e : entries)
        out.push_back(e.toUtf8().constData());
}

std::string KDbFileOperation::GetLastDirName(const std::string &path)
{
    return QDir(QString::fromStdString(path)).dirName().toUtf8().constData();
}

std::string KDbFileOperation::GetFileNameWithoutDir(const std::string &path)
{
    return QFileInfo(QString::fromStdString(path)).fileName().toUtf8().constData();
}

void KDbFileOperation::StringReplace(std::string &str, const std::string &from,
                                     const std::string &to)
{
    if (from.empty())
        return;
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos) {
        str.replace(pos, from.size(), to);
        pos += to.size();
    }
}

int KDbFileOperation::GetNumOfSpaces(const char *str, int n)
{
    // Реф. — вспомогательный парсер строк; точная роль второго аргумента из дизасма
    // не восстановлена. Принято: число пробелов ' ' среди первых n символов.
    if (str == nullptr || n <= 0)
        return 0;
    int cnt = 0;
    for (int i = 0; i < n && str[i] != '\0'; ++i)
        if (str[i] == ' ')
            ++cnt;
    return cnt;
}

bool KDbFileOperation::IsPatientDataExist(const std::string &path)
{
    return IsFileExist(path);   // реф. tail-call
}

void KDbFileOperation::GetCapacityByPath(const char *path, double &total, double &free)
{
    struct statfs st;
    if (path == nullptr || statfs(path, &st) != 0) {
        total = 0;
        free = 0;
        return;
    }
    total = static_cast<double>(st.f_blocks) * st.f_bsize;
    free  = static_cast<double>(st.f_bavail) * st.f_bsize;
}

void KDbFileOperation::GetSpecifyFreeCapacity(const char *path, double &free)
{
    double total = 0;
    GetCapacityByPath(path, total, free);
}

void KDbFileOperation::GetSpecifyTotalCapacity(const char *path, double &total)
{
    double free = 0;
    GetCapacityByPath(path, total, free);
}
