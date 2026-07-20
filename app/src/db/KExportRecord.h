#pragma once

#include <QFileInfo>
#include <QList>
#include <QMutex>
#include <QString>
#include <QStringList>

// Экспорт данных осмотра на USB-накопитель (реф. KExportRecord, X-2600).
//
// Статус экспорта. Из реверса достоверно восстановлены только два значения,
// которые класс присваивает сам; «успеха» он НЕ выставляет никогда — вызывающий
// трактует 0 как «ошибок не было».
enum _KSTATUS {
    K_EXPORT_IDLE       = 0,   // начальное
    K_EXPORT_COPY_FAIL  = 2,   // любой copyFile() != 1, включая выдёргивание USB
    K_EXPORT_NO_SPACE   = 4,   // не хватило места
};

class KExportRecord
{
public:
    // Реф. ctor(bool*): указатель на внешний флаг отмены; опрашивается в цикле
    // копирования (true ⇒ прервать).
    explicit KExportRecord(bool *pStopFlag);

    // --- пути на накопителе -------------------------------------------------
    // <usb>/Export/<sName>/   (KSystem::ExportPath уже оканчивается на '/')
    QString makeNameDirPath(QString &sName);
    // ВНИМАНИЕ, КВИРК ОРИГИНАЛА: аргумент подставляется ДВАЖДЫ —
    // makeNameDirPath(sId) даёт <usb>/Export/<sId>/, после чего к нему
    // приписывается ЕЩЁ РАЗ <sId>/ ⇒ итог `<usb>/Export/<S>/<S>/`.
    // Похоже на copy-paste-дефект вендора (в этом же классе живёт опечатка
    // setExprotStatus), но воспроизводится 1:1.
    QString makeExamIDPath(QString sId);
    // Для каждого id: каталог создаётся И очищается (существует и пуст).
    void    makeAllPath(QStringList &ids);
    // Реф.: НЕ рекурсивно; entryInfoList() с фильтрами по умолчанию (т.е. "."/".."
    // тоже перебираются — их удаление просто не удаётся); один sync() ПОСЛЕ цикла.
    void    cleanDir(QString &path);

    // --- отбор файлов -------------------------------------------------------
    // КВИРК ОРИГИНАЛА: внутри есть сравнение имени с "PicInfo.ini", но его
    // РЕЗУЛЬТАТ ОТБРАСЫВАЕТСЯ (нет removeAt) ⇒ фактически «нужны все обычные
    // файлы каталога». Мёртвая ветка сохранена.
    QList<QFileInfo> needCopy(QString &dirPath);

    // Удаляет в каталоге назначения файлы, ИМЕНА которых совпадают с исходными
    // (регистрозависимо, сравнение только по имени; без break во внутреннем
    // цикле), затем один sync(). Семантика — перезапись.
    void delExistFile(QString &destDir, QStringList srcFiles);

    // --- место --------------------------------------------------------------
    qint64 filesSize(QList<QFileInfo> &list);   // Σ размеров, байт
    qint64 freeSize();                          // свободно на КОРНЕ USB, байт
    // Реф.: (need + 1024) < free — беззнаковое СТРОГОЕ сравнение, запас ровно
    // 1024 байта. В сообщение об ошибке `need` печатается БЕЗ запаса.
    bool   IsSpaceEnough(QList<QFileInfo> &list);

    // --- собственно экспорт -------------------------------------------------
    // Реф.: 3-й и 4-й параметры НИКОГДА не читаются — сохранены для верности
    // сигнатуре. destDir должен оканчиваться на '/' (склейка простая).
    void ExportFiles(QList<QFileInfo> &files, QString &destDir,
                     const QString &unusedA, const QString &unusedB);
    void ExportFiles(QStringList &files, QString &destDir,
                     const QString &unusedA, const QString &unusedB);

    // --- состояние ----------------------------------------------------------
    int  GetSuccessFileNum();
    // КВИРК: в реф. m_examIdsNum после ctor НЕ ПИШЕТСЯ НИГДЕ — всегда 0.
    int  GetExamIdsNum();
    _KSTATUS ExportStatus() const;
    void setExprotStatus(const _KSTATUS &st);   // опечатка «Exprot» — из оригинала
    bool getIsEnoughSpace() const;
    void setIsEnoughSpace(bool v);
    QString filename();                         // под мьютексом
    void    setFilename(QString name);          // под мьютексом

private:
    int      m_successFileNum = 0;   // 0x00
    int      m_examIdsNum = 0;       // 0x04 — см. квирк выше
    bool     m_isEnoughSpace = true; // 0x08
    _KSTATUS m_status = K_EXPORT_IDLE; // 0x0c
    QString  m_filename;             // 0x10 — защищён мьютексом
    bool    *m_pStopFlag = nullptr;  // 0x18
    mutable QMutex m_mutex;          // 0x20 — защищает ТОЛЬКО m_filename
};
