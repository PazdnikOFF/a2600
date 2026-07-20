#include "db/KExportRecord.h"

#include <QDir>
#include <QFile>

#include <unistd.h>   // sync()

#include "db/KFileBackup.h"
#include "hal/KUsbDevice.h"
#include "kernel/KSystemLog.h"
#include "sys/KSystem.h"

namespace {
// Реф.: файл-метаданные экспорта. САМ KExportRecord его НЕ СОЗДАЁТ и не пишет —
// литерал встречается ровно в двух местах (needCopy, где результат сравнения
// отбрасывается, и ExportFiles, где он исключается из счётчика успехов).
// Кто его пишет — вне этого исполняемого файла (не найдено).
const char *const kPicInfoIni = "PicInfo.ini";

// Синтетический код реф. для «USB отключён» (не из KFileBackup).
const int kUsbDisconnected = -7;
} // namespace

KExportRecord::KExportRecord(bool *pStopFlag)
    : m_pStopFlag(pStopFlag), m_mutex(QMutex::NonRecursive)
{
}

// --- пути -------------------------------------------------------------------

QString KExportRecord::makeNameDirPath(QString &sName)
{
    const QString root = KSystem::ExportPath();
    if (root.isEmpty())
        return QString();
    if (!QDir().exists(root))
        QDir().mkdir(root);
    const QString path = root + sName + "/";
    if (!QDir().exists(path))
        QDir().mkpath(path);
    return path;
}

QString KExportRecord::makeExamIDPath(QString sId)
{
    QString p = makeNameDirPath(sId);
    if (p.isEmpty())
        return QString();
    // КВИРК ОРИГИНАЛА: тот же самый аргумент приписывается ВТОРОЙ раз ⇒
    // <usb>/Export/<sId>/<sId>/. Сохранено 1:1.
    p = p + sId + "/";
    if (!QDir().exists(p))
        QDir().mkpath(p);
    return p;
}

void KExportRecord::makeAllPath(QStringList &ids)
{
    for (int i = 0; i < ids.count(); ++i) {
        QString p = makeExamIDPath(ids[i]);
        if (QDir().exists(p))
            cleanDir(p);
        else
            QDir().mkpath(p);
    }
}

void KExportRecord::cleanDir(QString &path)
{
    QDir d(path);
    if (!d.exists(path))
        return;
    // Реф.: фильтры/сортировка ПО УМОЛЧАНИЮ ⇒ в список попадают "." и ".."
    // (их удаление просто не удаётся). Не рекурсивно.
    const QList<QFileInfo> entries = d.entryInfoList();
    for (const QFileInfo &fi : entries)
        QFile::remove(fi.absoluteFilePath());
    sync();   // реф.: один раз ПОСЛЕ цикла
}

// --- отбор ------------------------------------------------------------------

QList<QFileInfo> KExportRecord::needCopy(QString &dirPath)
{
    QDir dir(dirPath);
    dir.setFilter(QDir::Files);
    dir.setSorting(QDir::Name);
    QList<QFileInfo> result = dir.entryInfoList();
    for (int i = result.count() - 1; i >= 0; --i) {
        // КВИРК ОРИГИНАЛА: результат сравнения ОТБРАСЫВАЕТСЯ — исключения
        // PicInfo.ini фактически НЕ ПРОИСХОДИТ (мёртвый код сохранён).
        (void)result[i].fileName().compare(QLatin1String(kPicInfoIni), Qt::CaseSensitive);
    }
    return result;
}

void KExportRecord::delExistFile(QString &destDir, QStringList srcFiles)
{
    QDir dir(destDir);
    if (!dir.exists(destDir))
        return;
    const QList<QFileInfo> destList = dir.entryInfoList();
    QList<QFileInfo> srcInfos;
    for (const QString &s : srcFiles)
        srcInfos.append(QFileInfo(s));
    for (const QFileInfo &src : srcInfos)
        for (const QFileInfo &dst : destList)
            // Реф.: сравнение ТОЛЬКО по имени, регистрозависимо, без break.
            if (src.fileName().compare(dst.fileName(), Qt::CaseSensitive) == 0)
                QDir().remove(dst.absoluteFilePath());
    sync();
}

// --- место ------------------------------------------------------------------

qint64 KExportRecord::filesSize(QList<QFileInfo> &list)
{
    qint64 sum = 0;
    for (const QFileInfo &fi : list)
        sum += fi.size();
    return sum;
}

qint64 KExportRecord::freeSize()
{
    const QString p = KUsbDevice::GetInstance()->GetUsbPath();
    if (p.isEmpty())
        return 0;
    KFileBackup fb(nullptr);
    return fb.getDiskFreeSpace(p);   // реф.: на КОРНЕ USB, не на Export/
}

bool KExportRecord::IsSpaceEnough(QList<QFileInfo> &list)
{
    const qint64 free = freeSize();
    const qint64 need = filesSize(list);
    // Реф.: беззнаковое СТРОГОЕ сравнение, запас ровно 1024 байта.
    if (quint64(need + 1024) < quint64(free))
        return true;
    // Реф.: в сообщении `need` печатается БЕЗ запаса; апостроф — U+2019.
    LogPrintfEx(true, "[APP][E]: ",
                "U-drive space isn\xe2\x80\x99t enough. Avail: %dKB, Need: %dKB\n",
                int(free >> 10), int(need >> 10));
    return false;
}

// --- экспорт ----------------------------------------------------------------

void KExportRecord::ExportFiles(QList<QFileInfo> &files, QString &destDir,
                                const QString & /*unusedA*/, const QString & /*unusedB*/)
{
    if (!IsSpaceEnough(files)) {
        setExprotStatus(K_EXPORT_NO_SPACE);
        setIsEnoughSpace(false);
        return;
    }
    setIsEnoughSpace(true);
    if (files.count() <= 0)
        return;
    if (m_pStopFlag && *m_pStopFlag)
        return;                                  // проверка отмены ДО цикла

    for (int i = 0;;) {
        const QFileInfo fi = files[i];
        const QString name = fi.fileName();
        const QString src = fi.absoluteFilePath();
        const QString dst = destDir + name;      // простая склейка: destDir с '/'
        setFilename(name);

        int ret;
        if (KUsbDevice::GetInstance()->IsUsbDisconnect()) {
            ret = kUsbDisconnected;
        } else {
            KFileBackup fb(nullptr);
            ret = fb.copyFile(src, dst, true);   // реф.: overwrite = true, 1 = успех
        }

        if (ret == 1) {
            // Реф.: PicInfo.ini НЕ увеличивает счётчик и НЕ вызывает sync().
            if (name.compare(QLatin1String(kPicInfoIni), Qt::CaseSensitive) != 0) {
                ++m_successFileNum;
                sync();
            }
        } else {
            setExprotStatus(K_EXPORT_COPY_FAIL);
            const QString msg = QString("export : ") + QFileInfo(name).fileName()
                              + ", " + QString::number(fi.size(), 10) + "B, "
                              + "export failure";
            // Реф.: логируется тегом [I], а НЕ [E].
            QStringLogPrintf("[APP][I]: ", msg);
            if (ret == kUsbDisconnected)
                return;                          // USB выдернули — прерываем совсем
            sync();                              // прочие ошибки: продолжаем
        }

        if (++i >= files.count())
            break;
        if (m_pStopFlag && *m_pStopFlag)
            break;                               // проверка отмены на итерации
    }
    // Реф.: m_examIdsNum не трогается; «успешный» статус не выставляется.
}

void KExportRecord::ExportFiles(QStringList &files, QString &destDir,
                                const QString &unusedA, const QString &unusedB)
{
    QList<QFileInfo> list;
    for (const QString &s : files)
        list.append(QFileInfo(s));
    ExportFiles(list, destDir, unusedA, unusedB);
}

// --- состояние --------------------------------------------------------------

int KExportRecord::GetSuccessFileNum() { return m_successFileNum; }
int KExportRecord::GetExamIdsNum() { return m_examIdsNum; }
_KSTATUS KExportRecord::ExportStatus() const { return m_status; }
void KExportRecord::setExprotStatus(const _KSTATUS &st) { m_status = st; }
bool KExportRecord::getIsEnoughSpace() const { return m_isEnoughSpace; }
void KExportRecord::setIsEnoughSpace(bool v) { m_isEnoughSpace = v; }

QString KExportRecord::filename()
{
    m_mutex.lock();
    const QString v = m_filename;
    m_mutex.unlock();
    return v;
}

void KExportRecord::setFilename(QString name)
{
    m_mutex.lock();
    m_filename = name;
    m_mutex.unlock();
}
