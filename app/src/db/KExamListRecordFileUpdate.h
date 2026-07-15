#pragma once

#include <QString>
#include <QStringList>

// Пересчёт файлов записи осмотра (реф. KExamListRecordFileUpdate, X-2600).
// Считает файлы снимков/видео в каталоге осмотра по маскам расширений и
// (в оригинале) обновляет счётчики в tb_ExamList.
//
// Off-device-ядро: GetFiletypeNumFromPath — подсчёт файлов по name-фильтрам
// (QDir::entryInfoList). Расширения из прошивки: снимки *.jpg/*.bmp/*.png/*.jpeg,
// видео *.mp4/*.avi.
//
// ОТЛОЖЕНО (схема БД не подтверждена — НЕ фантазировать): UpdateRecordFileNumToDb —
// реф. читает запись (KExamListDBTableHandler::GetExamEntity), считает файлы в её
// каталоге и пишет счётчики обратно (UpdateExamEntity). В нашей схеме tb_ExamList
// колонок-счётчиков нет, и в строках бинарника их имена не найдены — колонка
// назначения неизвестна, поэтому DB-запись здесь не реализуется.
class KExamListRecordFileUpdate
{
public:
    // Число файлов в каталоге path, подходящих под маски exts (реф. 1:1:
    // QDir(path).entryInfoList(exts, Files)). Пустой/несуществующий путь → 0.
    static int GetFiletypeNumFromPath(const QString &path, const QStringList &exts);

    // Удобные обёртки на подтверждённых наборах расширений прошивки.
    static int ImageFileNum(const QString &dir);   // *.jpg/*.bmp/*.png/*.jpeg
    static int VideoFileNum(const QString &dir);    // *.mp4/*.avi
};
