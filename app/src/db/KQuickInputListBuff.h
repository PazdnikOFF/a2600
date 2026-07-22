#pragma once

#include <QString>

// Буфер совпадений быстрого ввода (реф. `_ListBuff`). Общий тип: его заполняют
// KQuickInput*DbTableHandler::GetMatchDate, а читает find-попап KQuickInputWidget
// (встроен в него по +0x48) — поэтому определение вынесено в отдельный заголовок.
//
// НЕ список строк: параллельные массивы ФИКСИРОВАННОЙ длины 10 + счётчик. Layout сверен
// дизасмом Get*/ClearListBuffData/SearchMatchItem:
//   Id[10]     std::string  +0x48  (шаг 0x20)
//   Name[10]   std::string  +0x188
//   Gender[10] int          +0x2c8 (шаг 4)
//   DoB[10]    std::string  +0x2f0 (формат "yyyy-MM-dd", литерал @0x85dc10)
//   Age[10]    int          +0x430
//   Count      int          +0x458
// Предел 10 записей зашит в каждый геттер попапа (`cmp w1, #0x9`).
struct _ListBuff
{
    enum { kMaxItems = 10 };

    QString Id[kMaxItems];
    QString Name[kMaxItems];
    int     Gender[kMaxItems];
    QString DoB[kMaxItems];
    int     Age[kMaxItems];
    int     Count = 0;

    _ListBuff() { Clear(); }
    // Реф. KQuickInputWidget::ClearListBuffData @0x692a70: строки — пустые, Gender=2 (!),
    // Age=0, Count=0.
    void Clear();
};
