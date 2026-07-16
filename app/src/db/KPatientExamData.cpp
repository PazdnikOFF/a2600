#include "db/KPatientExamData.h"

#include <QDir>
#include <QFileInfo>
#include <QString>

#include <algorithm>
#include <fstream>

namespace {
const char *REPORT_PDF = "report.pdf";

// Сортировка полных путей по QFileInfo::baseName() по возрастанию (реф. компаратор).
bool lessByBaseName(const std::string &a, const std::string &b)
{
    const QString ba = QFileInfo(QString::fromUtf8(a.c_str())).baseName();
    const QString bb = QFileInfo(QString::fromUtf8(b.c_str())).baseName();
    return ba < bb;
}
} // namespace

bool PatientExamData::IsFileExist(const std::string &path)
{
    // реф. — basic_filebuf::open(path, in) + good(); НЕ QFile::exists.
    std::ifstream f(path.c_str(), std::ios::in);
    return f.good();
}

int PatientExamData::GetExamDataPath(const std::string &examDir, std::vector<std::string> &out)
{
    if (examDir.empty())
        return -1;
    if (QDir(QString::fromUtf8(examDir.c_str())).exists())
        out.push_back(examDir);   // out НЕ очищается
    return 0;
}

int PatientExamData::GetExamData(const std::string &examDir, std::vector<std::string> &out,
                                 const QStringList &filters)
{
    if (examDir.empty())
        return -1;

    QDir dir(QString::fromUtf8(examDir.c_str()));
    if (!dir.exists())
        return -1;   // реф. — лог "dir(%s) is not exist", каталог не перечислен

    dir.setNameFilters(filters);
    dir.setSorting(QDir::Name);
    const QFileInfoList list = dir.entryInfoList();

    std::vector<std::string> names;
    for (const QFileInfo &fi : list)
        names.push_back(examDir + fi.fileName().toUtf8().constData());   // полный путь
    std::sort(names.begin(), names.end(), lessByBaseName);               // по baseName ↑

    out.insert(out.end(), names.begin(), names.end());                   // append
    return 0;
}

int PatientExamData::GetExamDataImage(const std::string &examDir, std::vector<std::string> &out)
{
    return GetExamData(examDir, out, QStringList() << "*.jpg" << "*.bmp");
}

int PatientExamData::GetExamDataVideo(const std::string &examDir, std::vector<std::string> &out)
{
    return GetExamData(examDir, out, QStringList() << "*.mkv" << "*.mp4");
}

int PatientExamData::GetExamDataPdf(const std::string &examDir, std::string &out)
{
    std::vector<std::string> dirs;
    const int rc = GetExamDataPath(examDir, dirs);
    if (rc)
        return -1;
    if (dirs.empty())
        return -1;
    const std::string p = dirs[0] + REPORT_PDF;
    if (IsFileExist(p))
        out = p;   // присваивается только если файл есть
    return rc;     // 0 даже если pdf отсутствует
}

int PatientExamData::GetExamDataAll(const std::string &examDir, std::vector<std::string> &out)
{
    const int rc1 = GetExamData(examDir, out, QStringList() << "*.jpg" << "*.bmp");
    std::string pdf;
    const int rc2 = GetExamDataPdf(examDir, pdf);
    if (rc2 == 0)
        out.push_back(pdf);           // реф. — append pdf при rc2==0 (без видео!)
    return (rc1 && rc2) ? -1 : 0;     // -1 только если ОБА провалились
}

bool PatientExamData::IsExamVideoExist(const std::string &examDir)
{
    QDir dir(QString::fromUtf8(examDir.c_str()));
    const QFileInfoList list = dir.entryInfoList(QStringList() << "*.mp4" << "*.mkv");
    return list.count() > 0;
}
