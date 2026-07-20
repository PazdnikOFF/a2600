#pragma once

#include <string>
#include <vector>

// Менеджер списка воспроизведения видео (реф. KVideoPlayerMng, X-2600;
// исходник реф. — dialog/videoplayer/KVideoPlayerMng.cpp).
//
// РЕВЕРС: синглтон-ОБЫЧНЫЙ объект — ни vtable, ни QObject; GetInstance() —
// pthread_once + static shared_ptr, наружу отдаётся СЫРОЙ указатель.
// sizeof == 0x40 (используется 0x3c).
//
// ⚠️ ИНВАРИАНТ ПОРЯДКА, определяющий ВСЮ семантику класса: AutoSetVideoFiles
// сортирует каталог как `QDir::Name | QDir::Reversed`, то есть **по убыванию
// имени**. Имена записей начинаются с метки времени ⇒ **индекс 0 — САМАЯ НОВАЯ
// запись**. Поэтому «Next» здесь означает УМЕНЬШЕНИЕ индекса для частей одной
// записи, а FindNextEnable сканирует ВПЕРЁД (к более старым записям).
// Это не ошибка — так в оригинале.

// Реф. KVideoListItem: ровно пять std::string, sizeof == 0xa0.
struct KVideoListItem {
    std::string fullPath;   // 0x00 абсолютный путь
    std::string group;      // 0x20 имя группы = baseName ДО ПОСЛЕДНЕГО '_'
    std::string index;      // 0x40 индекс части = baseName ПОСЛЕ последнего '_'
    std::string baseName;   // 0x60 QFileInfo::baseName()
    std::string suffix;     // 0x80 QFileInfo::suffix() В НИЖНЕМ РЕГИСТРЕ
};

class KVideoPlayerMng
{
public:
    static KVideoPlayerMng *GetInstance();

    int  GetPlayingStatus() const;

    // Реф.: пустой путь → лог стр. 49; не существует/не файл → стр. 62;
    // расширение не из набора → стр. 60. КВИРК: номера строк идут НЕ ПО ПОРЯДКУ
    // (62 у проверки существования, 60 у более поздней проверки расширения).
    bool IsValidVideoFile(const std::string &path);

    // Разбор имени «<группа>_<индекс>». Коды: 0 — успех, -1 — нет '_' либо
    // пустая половина, -2 — расширение не поддерживается.
    // КВИРК 1: путь декодируется как ASCII/Latin-1 (fromAscii_helper), тогда как
    //          все соседние методы используют fromUtf8.
    // КВИРК 2: out1/out2 записываются ДО проверки на пустоту, поэтому при
    //          возврате -1 они всё равно затираются.
    int  ParseSplitVideo(const std::string &in, std::string &out1, std::string &out2);

    // Заполнение списка из КАТАЛОГА переданного файла.
    // КВИРК: в фильтр entryList входит QDir::Dirs (0x610b) — каталог с именем
    // вида *.mp4 проходит фильтр и отсеивается только проверкой isFile().
    // КВИРК: код возврата ParseSplitVideo ОТБРАСЫВАЕТСЯ ⇒ файлы без '_'
    // попадают в список с пустыми group/index.
    // m_curPlayFile и m_playingStatus НЕ трогаются.
    void AutoSetVideoFiles(const std::string &filePath);
    void SetVideoFiles(const std::vector<KVideoListItem> &v);

    int            GetVideoListItemIndexByPath(const std::string &p);   // первое совпадение, -1
    KVideoListItem GetVideoListItemByPath(const std::string &p);        // копия; пустая, если нет

    // Поиск соседней ЗАПИСИ (иной группы). Last — назад по индексу, Next — вперёд.
    bool FindLastEnable(int idx, std::string &out);
    bool FindLastEnable(int idx);
    // Реф.: возвращает элемент с НАИБОЛЬШИМ индексом внутри следующей отличной
    // группы (= хронологически самую раннюю часть той записи).
    bool FindNextEnable(int idx, std::string &out);
    bool FindNextEnable(int idx);

    // КВИРК: FindLastVideo берёт ПОСЛЕДНЕЕ совпадение пути (собственный скан без
    // выхода из цикла), а FindNextVideo — ПЕРВОЕ (через GetVideoListItemIndexByPath).
    // При дублирующихся путях направления разрешаются по-разному.
    bool FindLastVideo(const std::string &p, std::string &out);
    bool FindNextVideo(const std::string &p, std::string &out);
    // КВИРК: группа НЕ проверяется вообще — просто сосед с индексом idx-1.
    bool FindNextSplitVideo(const std::string &p, std::string &out);

    // КВИРК: скан НЕ прерывается на совпадении, выходящем за границу, поэтому
    // последний (соотв. нулевой) элемент никогда не даёт результата.
    std::string GetNextVideoFileFullPath();
    std::string GetPreVideoFileFullPath();

    std::string SwitchNextVideoFileFullPath();
    std::string SwitchLastVideoFileFullPath();
    std::string SwitchNextSplitVideoFileFullPath();

    // КВИРК: вопреки имени, out-параметры означают «есть предыдущая / есть
    // следующая запись», то есть ОТРИЦАНИЕ «первая»/«последняя».
    // Список из РОВНО ОДНОГО элемента даёт false/false.
    void CheckIsFirstOrLast(const std::string &p, bool &hasPrev, bool &hasNext);

    // true, если предыдущий по списку файл принадлежит ТОЙ ЖЕ группе разбиения
    // (значит запись продолжается и надо играть дальше автоматически).
    bool CheckIsNeedPlayNextVideo();

    // Единственный device/UI-метод (KVideoPlayerOSD + modetest) — off-device
    // выполняется только его логическая часть.
    void PlayVideo(const std::string &p, bool autoSet);

    // Не из реф. — доступ для self-test.
    const std::vector<KVideoListItem> &VideoList() const { return m_videoList; }
    const std::string &CurPlayFile() const { return m_curPlayFile; }
    void SetCurPlayFile(const std::string &p) { m_curPlayFile = p; }

private:
    KVideoPlayerMng() = default;

    std::string                 m_curPlayFile;      // 0x00
    std::vector<KVideoListItem> m_videoList;        // 0x20
    int                         m_playingStatus = 0;// 0x38
};
