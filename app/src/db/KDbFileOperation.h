#pragma once

#include <string>
#include <vector>

// Файловые/дисковые утилиты (реф. KDbFileOperation, X-2600). Несмотря на «Db» в
// имени — SQLite НЕ трогает: это чистые ФС/statfs-хелперы поверх Qt+POSIX.
// Нет ctor/dtor/vtable → все методы static, полей нет. НЕ UI, НЕ device.
//
// Пересечения с KEntityService нет (тот — PRAGMA/backup .dat через SQLite).
class KDbFileOperation
{
public:
    static bool IsFileExist(const std::string &path);          // QFile::exists
    static bool IsFileDirExist(const std::string &path);       // существует (файл ИЛИ каталог)
    static long long GetFileSize(const std::string &path);     // размер файла (байт), -1 если нет
    static bool RemoveFile(const std::string &path);           // QFile::remove
    static bool CreateFolder(const std::string &path);         // QDir::mkpath
    static bool DeleteFolder(const std::string &path);         // рекурсивное удаление
    static bool CopyFileToFile(const std::string &src, const std::string &dst);

    // Листинг каталога по маске (QDir::entryList). filter — glob ("*.jpg").
    static void GetFilesByFilter(const std::string &dir, const std::string &filter,
                                 std::vector<std::string> &out);

    static std::string GetLastDirName(const std::string &path);        // последний компонент
    static std::string GetFileNameWithoutDir(const std::string &path); // basename

    // Замена ВСЕХ вхождений from→to (реф. — правит строку на месте).
    static void StringReplace(std::string &str, const std::string &from, const std::string &to);

    // Число пробелов ' ' в первых n символах C-строки (реф. — вспомогательный парсер;
    // точная роль второго аргумента из дизасма не восстановлена — см. .cpp).
    static int GetNumOfSpaces(const char *str, int n);

    static bool IsPatientDataExist(const std::string &path);   // реф. tail-call → IsFileExist

    // Ёмкость ФС по пути (statfs), в БАЙТАХ (реф. f_blocks·f_bsize / f_bavail·f_bsize).
    static void GetCapacityByPath(const char *path, double &total, double &free);
    static void GetSpecifyFreeCapacity(const char *path, double &free);
    static void GetSpecifyTotalCapacity(const char *path, double &total);
};
