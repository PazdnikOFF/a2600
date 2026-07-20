#pragma once

#include <string>

// Отметка времени (реф. KTimeInfo, X-2600), sizeof == 0xE0, без vtable.
//
// ВАЖНО (реверс): это СНИМОК, а не «живые часы». Init() зовётся ТОЛЬКО из
// конструктора, публичного Update()/Refresh() нет — на каждую новую отметку
// оригинал создаёт новый объект. Все геттеры — методы экземпляра.
//
// Init(): std::chrono::system_clock::now() → ns; secs = ns/1e9; localtime(&secs)
// (именно localtime, НЕ localtime_r и НЕ QDateTime); msec = ns/1e6 - secs*1000.
// Компоненты формируются snprintf в буфер 16 байт. Qt не используется вообще.
//
// `sep` в геттерах передаётся ПО ЗНАЧЕНИЮ (std::string), вставляется МЕЖДУ
// компонентами; пустой sep даёт слитную форму ("20260720").
class KTimeInfo
{
public:
    KTimeInfo();                               // поля = "", затем Init()

    void Init();

    std::string GetCurrentTimeYYYY() const;                     // m_strYear
    std::string GetCurrentTimeYYYYMM(std::string sep) const;
    std::string GetCurrentTimeYYYYMMDD(std::string sep) const;
    std::string GetCurrentTimeHHMMSS(std::string sep) const;
    // Реф.: YYYYMMDD(dateSep) + "_" + HHMMSS(timeSep); '_' ЗАШИТ (одиночный литерал).
    std::string GetCurrentTimeYYYYMMDD_HHMMSS(std::string dateSep,
                                              std::string timeSep) const;

    // Подмена времени для self-test (не из реф.): != 0 → используется как
    // «сейчас» в UNIX-секундах.
    static void SetFixedTime(long long unixSeconds);

private:
    std::string m_strYear;    // 0x00  "%04d" (tm_year+1900)
    std::string m_strMonth;   // 0x20  "%02d" (tm_mon+1)
    std::string m_strDay;     // 0x40  "%02d"
    std::string m_strHour;    // 0x60  "%02d"
    std::string m_strMin;     // 0x80  "%02d"
    std::string m_strSec;     // 0xa0  "%02d"
    std::string m_strMsec;    // 0xc0  "%03d" — заполняется, но геттера в реф. НЕТ

    static long long s_fixedTime;
};

// Монотонные миллисекунды для замеров «Use time %lld ms» (реф. — свободная
// функция GetMs(), не метод KTimeInfo).
long long GetMs();
