#pragma once

#include <QString>

// Именование/нумерация файлов снимков и видео (реф. KSaveFile, X-2600).
// Off-device ядро — «поточный номер» файла (flow number) и разбор имён:
//   • FormatFlowNumber(n) — 3-значный zero-padded номер ("001".."999"), overflow → "999^"
//     (1:1 с бинарником: строки "000"/"001"/"999^" в GetFileFlowNumber);
//   • расширения: снимок = "jpg", видео = "mp4";
//   • FlowNumberFromName / FindMaxFileFlowNumber — извлечение номера из имени и
//     поиск максимума в каталоге (реф. GetFileFlowNumber/FindMaxFileFlowNumber —
//     сканирует существующие файлы, чтобы выдать следующий свободный номер).
//
// Полная CreateFilePath (дата/серийник/examID под endodata/…) — device (KTime/
// KProductsSerial/KExamBussinessHandler), Фаза E. Здесь — чистая логика номеров.
class KSaveFile
{
public:
    static const int  kMaxFlow = 999;         // потолок номера (реф. "999^")
    static const char kOverflowMark = '^';    // маркер переполнения

    // Форматирование поточного номера: [0..999] → "000".."999"; >999 → "999^".
    static QString FormatFlowNumber(int n);

    // Извлечь номер из имени файла ("<flow>.<ext>" → flow). -1, если не распознан.
    // Понимает как "001", так и overflow "999^" (→ 999).
    static int FlowNumberFromName(const QString &fileName);

    // Расширение по типу (реф.: jpg/mp4).
    static QString ImageExt() { return "jpg"; }
    static QString VideoExt() { return "mp4"; }

    // Имя файла по номеру и типу: "<flow>.<ext>".
    static QString MakeFileName(int flow, bool video);

    // Максимальный номер среди файлов *.jpg/*.mp4 в каталоге (-1, если файлов нет).
    static int FindMaxFileFlowNumber(const QString &dir);
    // Следующий свободный номер (max+1), но не выше kMaxFlow (реф.: потолок 999).
    static int NextFlowNumber(const QString &dir);
};
