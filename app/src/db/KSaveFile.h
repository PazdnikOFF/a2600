#pragma once

#include <QString>

// Именование/нумерация файлов снимков и видео (реф. KSaveFile, X-2600).
// Off-device ядро — «поточный номер» файла (flow number) и разбор имён.
//
// 1:1 с бинарником (aarch64, update/root/X2000):
//   • _ZN9KSaveFile16FormatFlowNumberB5cxx11Ei @0x6a89b8 (0x350) — это ЧИСТЫЙ
//     zero-pad до 3 знаков через std::stringstream: width(3) + fill('0'),
//     `ss << n; ss >> str`. Никакого сравнения с 999 и никакого '^' внутри
//     функции нет (в теле отсутствует константа 0x3e7 и нет ссылок на .rodata).
//   • Маркер переполнения добавляет ВЫЗЫВАЮЩИЙ — GetFileFlowNumber @0x6a99d8:
//     .rodata "999^" @0x8695f8 и "001" @0x869600, вставка "999^" (len 4) в
//     позицию 0 перед результатом FormatFlowNumber → имена вида "999^001".
//     Совпадает с KExamDataFileNameGenerator::GetFileSerialNum @0x48BEA8
//     ("%d^%03d", где первый %d — литеральная 999).
//   • CheckIsFileNumberUseUp @0x6a92c8 сравнивает с .rodata "999^999" @0x8695d0 —
//     это последнее допустимое имя, после него нумерация исчерпана.
//   • FindMaxFileFlowNumber @0x6ab6d0 — regex'ы "([0-9]{2}[1-9]{1}|...)",
//     "(.jpg|(_[0-9]{3}.mp4))", "([\^]{1})", "999" — распознаёт обе формы.
//
// Полная CreateFilePath (дата/серийник/examID под endodata/…) — device (KTime/
// KProductsSerial/KExamBussinessHandler), Фаза E. Здесь — чистая логика номеров.
class KSaveFile
{
public:
    static const int  kMaxFlow = 999;          // потолок «первого круга» нумерации
    static const char kOverflowMark = '^';     // маркер переполнения (реф. "999^")
    // Линейный потолок: "999^999" = 999 + 999.
    static const int  kMaxFlowOverall = kMaxFlow + kMaxFlow;

    static QString OverflowPrefix() { return QString("%1%2").arg(kMaxFlow).arg(kOverflowMark); }
    static QString UseUpFlowName()  { return OverflowPrefix() + QString::number(kMaxFlow); }

    // Реф. FormatFlowNumber: zero-pad до 3 знаков, БЕЗ потолка и БЕЗ '^'
    // (stringstream width(3)/fill('0'), поэтому n>999 даёт натуральные цифры,
    // а отрицательное — левый паддинг: -5 → "0-5").
    static QString FormatFlowNumber(int n);

    // Имя-номер целиком, как его собирает GetFileFlowNumber:
    //   n <= 999 → "001".."999"
    //   n >  999 → "999^" + FormatFlowNumber(n % 999, 0→999) → "999^001".."999^999"
    static QString MakeFlowName(int n);

    // Извлечь номер из имени файла ("<flow>.<ext>" → flow). -1, если не распознан.
    // "003.jpg" → 3, "999^001.jpg" → 1000 (999 + 1) — нумерация остаётся монотонной.
    static int FlowNumberFromName(const QString &fileName);

    // Реф. CheckIsFileNumberUseUp: номера исчерпаны, если имя-номер == "999^999".
    static bool CheckIsFileNumberUseUp(const QString &fileName);

    // Расширение по типу (реф.: jpg/mp4).
    static QString ImageExt() { return "jpg"; }
    static QString VideoExt() { return "mp4"; }

    // Имя файла по номеру и типу: "<flow>.<ext>".
    static QString MakeFileName(int flow, bool video);

    // Максимальный номер среди файлов *.jpg/*.mp4 в каталоге (-1, если файлов нет).
    static int FindMaxFileFlowNumber(const QString &dir);
    // Следующий свободный номер (max+1), но не выше kMaxFlowOverall ("999^999").
    static int NextFlowNumber(const QString &dir);
};
