#pragma once

#include <QStringList>

#include <string>
#include <vector>

// Перечисление файлов данных обследования (реф. PatientExamData, файл
// dialog/patient/KPatientExamData.cpp, X-2600). ВСЕ методы STATIC (класс без состояния
// и без vtable — реф. передаёт id в x0 без this).
//
// РАЗЛИЧИЕ С РЕФЕРЕНСОМ (device-only вырезано): реф. резолвит каталог обследования из id
// как GetUsbDevice().GetUsbPath() + KExamListDBTableHandler::GetExamEntity(id).dir — это
// USB-монтирование + on-device SQLite, off-device не воспроизводимо. Здесь методы принимают
// ГОТОВЫЙ каталог обследования examDir напрямую (должен оканчиваться на '/', как entry.dir).
// Вся файловая логика (QDir name-filters, полные пути, сортировка по baseName, append) —
// сверена дизасмом 1:1.
class PatientExamData
{
public:
    // ifstream open + good() (реф. — НЕ QFile::exists): true, если файл открылся на чтение.
    static bool IsFileExist(const std::string &path);

    // Есть ли видео в каталоге: QDir(examDir).entryInfoList({"*.mp4","*.mkv"}).count() > 0.
    static bool IsExamVideoExist(const std::string &examDir);

    // examDir + "report.pdf"; out присваивается ТОЛЬКО если файл существует. Возврат 0/-1.
    static int GetExamDataPdf(const std::string &examDir, std::string &out);

    // Каталог(и) обследования, если существует (реф. push_back examDir). out НЕ очищается.
    static int GetExamDataPath(const std::string &examDir, std::vector<std::string> &out);

    // Ядро-перечислитель по name-фильтрам. Полные пути (examDir+fileName), сортировка по
    // QFileInfo::baseName() по возрастанию, out НЕ очищается (append). Возврат 0/-1.
    static int GetExamData(const std::string &examDir, std::vector<std::string> &out,
                           const QStringList &filters);

    // Картинки *.jpg/*.bmp (реф. — без .png).
    static int GetExamDataImage(const std::string &examDir, std::vector<std::string> &out);
    // Видео *.mkv/*.mp4 (реф. — без .avi/.mov).
    static int GetExamDataVideo(const std::string &examDir, std::vector<std::string> &out);
    // Все картинки (*.jpg/*.bmp) + report.pdf (реф. — БЕЗ видео). -1 только если оба провалились.
    static int GetExamDataAll(const std::string &examDir, std::vector<std::string> &out);
};
