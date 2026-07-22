#include "video/KVideoPlayerMng.h"

#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QString>
#include <QStringList>

#include <mutex>

#include "kernel/KSystemLog.h"
#include <memory>   // std::shared_ptr/unique_ptr (libstdc++ не тянет транзитивно)

namespace {

// Реф. файловые статики (_GLOBAL__sub_I_KVideoPlayerMng.cpp @0x220810).
const char *const kSrcFile = "dialog/videoplayer/KVideoPlayerMng.cpp";
const char *const kInfo    = "[info]";

const QSet<QString> &videoExtensionFilter()
{
    static const QSet<QString> s = {
        QStringLiteral("mp4"), QStringLiteral("mkv"),
        QStringLiteral("flv"), QStringLiteral("avi"),
    };
    return s;
}

const QStringList &videoNameFilter()
{
    static const QStringList l = {
        QStringLiteral("*.mp4"), QStringLiteral("*.mkv"),
        QStringLiteral("*.flv"), QStringLiteral("*.avi"),
    };
    return l;
}

} // namespace

KVideoPlayerMng *KVideoPlayerMng::GetInstance()
{
    static std::once_flag once;
    static std::shared_ptr<KVideoPlayerMng> m_instance;
    std::call_once(once, [] {
        m_instance = std::shared_ptr<KVideoPlayerMng>(new KVideoPlayerMng(),
                                                      [](KVideoPlayerMng *p) { delete p; });
        // Реф. ctor также зовёт свободную InitGst() (pthread_once + gst_init) —
        // off-device пропускаем.
    });
    return m_instance.get();
}

int KVideoPlayerMng::GetPlayingStatus() const { return m_playingStatus; }

// --- проверка файла ---------------------------------------------------------

bool KVideoPlayerMng::IsValidVideoFile(const std::string &path)
{
    if (path.empty()) {
        LogPrintfx(kInfo, kSrcFile, 49, "IsValidVideoFile", "File path is null\n");
        return false;
    }
    const QFileInfo fi(QString::fromUtf8(path.c_str()));
    if (!fi.exists() || !fi.isFile()) {
        // КВИРК реф.: строка 62 — раньше по коду, чем строка 60 ниже.
        LogPrintfx(kInfo, kSrcFile, 62, "IsValidVideoFile", "File is not exist%s\n",
                   path.c_str());
        return false;
    }
    if (!videoExtensionFilter().contains(fi.suffix().toLower())) {
        LogPrintfx(kInfo, kSrcFile, 60, "IsValidVideoFile",
                   "File extension is not supported:%s\n", path.c_str());
        return false;
    }
    return true;
}

// --- разбор имени части -----------------------------------------------------

int KVideoPlayerMng::ParseSplitVideo(const std::string &in, std::string &out1,
                                     std::string &out2)
{
    // КВИРК реф.: здесь fromAscii_helper (Latin-1), а не fromUtf8, как у соседей.
    const QString that = QString::fromLatin1(in.c_str());
    const QFileInfo fi(that);
    if (!videoExtensionFilter().contains(fi.suffix().toLower()))
        return -2;                       // out1/out2 не тронуты

    const QString base = fi.baseName();
    const int idx = base.lastIndexOf(QLatin1Char('_'), -1, Qt::CaseSensitive);
    if (idx == -1)
        return -1;

    // КВИРК реф.: запись происходит ДО проверки на пустоту ⇒ при возврате -1
    // out-параметры всё равно затираются.
    out1 = base.left(idx).toUtf8().constData();
    out2 = base.mid(idx + 1).toUtf8().constData();
    if (out1.empty())
        return -1;
    if (out2.empty())
        return -1;
    return 0;
}

// --- наполнение списка ------------------------------------------------------

void KVideoPlayerMng::AutoSetVideoFiles(const std::string &filePath)
{
    const QFileInfo fi(QString::fromUtf8(filePath.c_str()));
    if (!fi.exists() || !fi.isFile()) {
        LogPrintfx(kInfo, kSrcFile, 116, "AutoSetVideoFiles",
                   "KVideoPlayerMng::AutoSetVideoFiles: current file is not valid\n");
        return;                          // список НЕ трогается
    }
    QDir dir = fi.absoluteDir();
    // Реф. фильтры 0x610b = Dirs|Files|NoSymLinks|Hidden|NoDotAndDotDot,
    // сортировка 0x8 = Name|Reversed (ПО УБЫВАНИЮ ⇒ индекс 0 — самое новое).
    // КВИРК: Dirs в фильтре — каталог с именем *.mp4 пройдёт и отсеется лишь
    // проверкой isFile() ниже.
    const QStringList entries = dir.entryList(
        videoNameFilter(),
        QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::Hidden | QDir::NoDotAndDotDot,
        QDir::Name | QDir::Reversed);

    m_videoList.clear();
    for (const QString &e : entries) {
        KVideoListItem it;
        it.fullPath = dir.absoluteFilePath(e).toUtf8().constData();
        const QFileInfo fi2(QString::fromUtf8(it.fullPath.c_str()));
        if (!fi2.exists() || !fi2.isFile())
            continue;                    // каталоги отсеиваются здесь
        // КВИРК реф.: код возврата ОТБРАСЫВАЕТСЯ — файлы без '_' попадают
        // в список с пустыми group/index.
        ParseSplitVideo(it.fullPath, it.group, it.index);
        it.baseName = fi2.baseName().toUtf8().constData();
        it.suffix   = fi2.suffix().toLower().toUtf8().constData();
        m_videoList.push_back(it);
    }
    LogPrintfx(kInfo, kSrcFile, 138, "AutoSetVideoFiles",
               "Video Player File List size: %d", int(m_videoList.size()));
}

void KVideoPlayerMng::SetVideoFiles(const std::vector<KVideoListItem> &v)
{
    // Реф.: размер берётся у АРГУМЕНТА и логируется ДО присваивания.
    LogPrintfx(kInfo, kSrcFile, 167, "SetVideoFiles",
               "Video Player File List size: %d", int(v.size()));
    m_videoList = v;
}

// --- поиск по списку --------------------------------------------------------

int KVideoPlayerMng::GetVideoListItemIndexByPath(const std::string &p)
{
    for (size_t i = 0; i < m_videoList.size(); ++i)
        if (m_videoList[i].fullPath == p)
            return int(i);               // ПЕРВОЕ совпадение
    return -1;
}

KVideoListItem KVideoPlayerMng::GetVideoListItemByPath(const std::string &p)
{
    for (const KVideoListItem &it : m_videoList)
        if (it.fullPath == p)
            return it;
    return KVideoListItem();             // пустая при отсутствии/пустом списке
}

bool KVideoPlayerMng::FindLastEnable(int idx, std::string &out)
{
    out.clear();
    if (idx <= 0 || idx >= int(m_videoList.size()))
        return false;
    const std::string &grp = m_videoList[idx].group;
    for (int j = idx - 1; j >= 0; --j) {
        if (m_videoList[j].group != grp) {
            out = m_videoList[j].fullPath;
            return true;
        }
    }
    return false;
}

bool KVideoPlayerMng::FindLastEnable(int idx)
{
    std::string tmp;
    return FindLastEnable(idx, tmp);
}

bool KVideoPlayerMng::FindNextEnable(int idx, std::string &out)
{
    out.clear();
    const int n = idx + 1;
    if (idx < 0 || int(m_videoList.size()) <= n)
        return false;
    const std::string &grp = m_videoList[idx].group;

    // Найти первую отличную группу, начиная с n.
    int k = -1;
    for (int j = n; j < int(m_videoList.size()); ++j) {
        if (m_videoList[j].group != grp) {
            k = j;
            break;
        }
    }
    if (k < 0)
        return false;

    // Реф. собирает во ВРЕМЕННЫЙ вектор все подряд идущие элементы этой группы
    // и берёт ПОСЛЕДНИЙ; результат тот же, что и просто досканировать до конца
    // группы (сам вектор — чистые накладные расходы).
    const std::string target = m_videoList[k].group;
    int last = -1;
    for (int j = k; j < int(m_videoList.size()); ++j) {
        if (m_videoList[j].group != target)
            break;
        last = j;
    }
    if (last < 0)
        return false;
    out = m_videoList[last].fullPath;
    return true;
}

bool KVideoPlayerMng::FindNextEnable(int idx)
{
    std::string tmp;
    return FindNextEnable(idx, tmp);
}

bool KVideoPlayerMng::FindLastVideo(const std::string &p, std::string &out)
{
    out.clear();
    // КВИРК реф.: собственный скан БЕЗ выхода из цикла ⇒ запоминается
    // ПОСЛЕДНЕЕ совпадение (в отличие от FindNextVideo, берущего первое).
    int idx = -1;
    for (size_t i = 0; i < m_videoList.size(); ++i)
        if (m_videoList[i].fullPath == p)
            idx = int(i);
    if (idx == -1)
        return false;
    return FindLastEnable(idx, out);
}

bool KVideoPlayerMng::FindNextVideo(const std::string &p, std::string &out)
{
    out.clear();
    const int idx = GetVideoListItemIndexByPath(p);   // ПЕРВОЕ совпадение
    if (idx == -1)
        return false;
    return FindNextEnable(idx, out);
}

bool KVideoPlayerMng::FindNextSplitVideo(const std::string &p, std::string &out)
{
    out.clear();
    const int idx = GetVideoListItemIndexByPath(p);
    if (idx == -1)
        return false;
    if (idx - 1 < 0)
        return false;
    // Реф.: группа НЕ проверяется — просто сосед слева.
    out = m_videoList[idx - 1].fullPath;
    return true;
}

std::string KVideoPlayerMng::GetNextVideoFileFullPath()
{
    // КВИРК реф.: скан НЕ прерывается, поэтому совпадение на ПОСЛЕДНЕМ индексе
    // не даёт результата (idx+1 вышел бы за границу).
    int idx = -1;
    for (size_t i = 0; i + 1 < m_videoList.size(); ++i)
        if (m_videoList[i].fullPath == m_curPlayFile)
            idx = int(i);
    if (idx < 0)
        return std::string();
    return m_videoList[idx + 1].fullPath;
}

std::string KVideoPlayerMng::GetPreVideoFileFullPath()
{
    // КВИРК реф.: совпадение на индексе 0 пропускается, скан продолжается.
    int idx = -1;
    for (size_t i = 1; i < m_videoList.size(); ++i)
        if (m_videoList[i].fullPath == m_curPlayFile)
            idx = int(i);
    if (idx < 1)
        return std::string();
    return m_videoList[idx - 1].fullPath;
}

std::string KVideoPlayerMng::SwitchNextVideoFileFullPath()
{
    std::string t;
    if (FindNextVideo(m_curPlayFile, t)) {
        m_curPlayFile = t;
        return t;
    }
    return std::string();
}

std::string KVideoPlayerMng::SwitchLastVideoFileFullPath()
{
    std::string t;
    if (FindLastVideo(m_curPlayFile, t)) {
        m_curPlayFile = t;
        return t;
    }
    return std::string();
}

std::string KVideoPlayerMng::SwitchNextSplitVideoFileFullPath()
{
    std::string t;
    if (FindNextSplitVideo(m_curPlayFile, t)) {
        m_curPlayFile = t;
        return t;
    }
    return std::string();
}

void KVideoPlayerMng::CheckIsFirstOrLast(const std::string &p, bool &hasPrev, bool &hasNext)
{
    // Реф.: пустой список ИЛИ список ровно из ОДНОГО элемента → false/false.
    if (m_videoList.size() <= 1) {
        hasPrev = false;
        hasNext = false;
        return;
    }
    const int i = GetVideoListItemIndexByPath(p);
    if (i < 0)
        return;   // НЕ УСТАНОВЛЕНО: похоже, оба параметра остаются нетронутыми
    if (i == 0) {
        hasPrev = false;
        hasNext = FindNextEnable(0);
    } else if (i == int(m_videoList.size()) - 1) {
        hasPrev = FindLastEnable(i);
        hasNext = false;
    } else {
        hasPrev = FindLastEnable(i);
        hasNext = FindNextEnable(i);
    }
}

bool KVideoPlayerMng::CheckIsNeedPlayNextVideo()
{
    const KVideoListItem cur = GetVideoListItemByPath(m_curPlayFile);
    const std::string prevPath = GetPreVideoFileFullPath();
    const KVideoListItem prev = GetVideoListItemByPath(prevPath);
    LogPrintfx(kInfo, kSrcFile, 261, "CheckIsNeedPlayNextVideo", "Current:%s, next:%s\n",
               cur.fullPath.c_str(), prev.fullPath.c_str());
    if (prev.fullPath.empty())
        return false;
    return prev.group == cur.group;      // та же группа ⇒ запись продолжается
}

void KVideoPlayerMng::PlayVideo(const std::string &p, bool autoSet)
{
    m_curPlayFile = p;
    if (autoSet)
        AutoSetVideoFiles(p);
    // ДАЛЬШЕ — device/UI (KVideoPlayerOSD-диалог + system("modetest -D
    // fd4a0000.zynqmp-display -w 36:alpha:255")); off-device только статус.
    m_playingStatus = 1;
    m_playingStatus = 0;
}
