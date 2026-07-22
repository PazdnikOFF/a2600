#pragma once

#include <QDateTime>

#include <string>

// Элемент модели быстрого ввода (реф. KComboBoxItem, sizeof 0x30 — dtor @0x5aa4b0,
// copy-ctor заинлайнен в vector<KComboBoxItem>::_M_realloc_insert @0x5abe18).
// Layout сверен дизасмом:
//   +0x00 int         — mKey сущности (entity +0x00)
//   +0x04 int         — count сущности (entity +0x74 у пациента)
//   +0x08 QDateTime   — время; см. примечание ниже
//   +0x10 std::string — текст элемента (он же ключ поиска в SetData)
//
// ИМЕНА ПОЛЕЙ РЕФ. НЕ ВОССТАНОВЛЕНЫ (символов нет, ctor/operator= заинлайнены) — здесь
// они названы по семантике, которую видно в местах заполнения.
//
// ПРИМЕЧАНИЕ (реф.-странность, сохранена как факт, а не воспроизведена):
// KQuickInputDataPatientName::GetData @0x5ac520 ПАРСИТ время сущности
// (QDateTime::fromString(entity.time, "yyyy-MM-dd hh:mm:ss"), формат-литерал @0x83df48),
// но результат в +0x08 НЕ КЛАДЁТ — туда копируется дефолтный (невалидный) QDateTime.
// Похоже на мёртвый код. В порте поле заполняется распарсенным значением: это полезнее
// и ничего не ломает (модель его всё равно не показывает — см. KQuickInputModel::data).
struct KComboBoxItem
{
    int         mKey = -1;    // +0x00
    int         count = 0;    // +0x04
    QDateTime   time;         // +0x08
    std::string text;         // +0x10
};
