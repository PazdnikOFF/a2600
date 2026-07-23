#pragma once

#include <map>
#include <string>

#include "report/KReportTemplateData.h"

class KRTTeCreatorContext;
class QTextTableCell;

// Базовый «творец» блока в Te-движке (реф. KRTTeAbsItemCreator, ctor @0x513260).
// Te-движок рендерит отчёт в QTextDocument (и на печать), в отличие от Simple-движка,
// который лишь ВЫЧИСЛЯЕТ доступность элементов.
//
// Реф. ctor — три инструкции: m_curItemConfig = nullptr, vptr, m_context = &ctx.
// Реф. `CreateItem` @0x513238 — `assert(false)` (литерал "false" @0x85f850, файл
// .../texteditcreator/KRTTe...) ⇒ в порте чисто виртуальный.
class KRTTeAbsItemCreator
{
public:
    explicit KRTTeAbsItemCreator(KRTTeCreatorContext &ctx) : m_context(ctx) {}
    virtual ~KRTTeAbsItemCreator() = default;

    virtual int CreateItem(const KReportTemplateItem &item,
                           const std::map<std::string, std::string> &cfgMap,
                           QTextTableCell &cell) = 0;

    // ⭐ Реф. @0x513530 — ГЛАВНЫЙ ФИЛЬТР Te-движка и ЗАМЫКАНИЕ СВЯЗКИ С Simple-движком.
    // Логика (сверена дизасмом):
    //   • cfgMap ПУСТА → IsItemValid(имя элемента) @0x506a38; false → создание отменяется;
    //   • cfgMap НЕ пуста → ищется ключ `PageBreak_Before` (@0x866000):
    //       – НЕ найден → сразу UpdateItemConfigPointer + true (валидность НЕ проверяется!);
    //       – найден    → из значения строятся source-id через ConvertToSourceID и
    //                     IsItemValid зовётся уже с ЭТИМ ключом; false → отмена.
    //   • во всех true-ветках ПОСЛЕДНИМ действием идёт UpdateItemConfigPointer(item);
    //     в false-ветках он НЕ вызывается.
    bool CheckCreate(const KReportTemplateItem &item,
                     const std::map<std::string, std::string> &cfgMap);

    // Реф. @0x5133a0: заголовок берётся ТОЛЬКО через tr() от item.m_strTitle
    // (получатель — QObject::staticMetaObject). Второй аргумент (карта) НЕ используется
    // вообще, m_curItemConfig тоже не читается.
    std::string GetItemTitle(const KReportTemplateItem &item,
                             const std::map<std::string, std::string> &unused) const;

    // Реф. @0x513278: сбрасывает кэш и ищет конфиг элемента в карте контекста;
    // не нашёл — остаётся nullptr.
    void UpdateItemConfigPointer(const KReportTemplateItem &item);

    const KReportTemplateItemConfig *CurItemConfig() const { return m_curItemConfig; }

protected:
    KRTTeCreatorContext             &m_context;                 // +0x08
    const KReportTemplateItemConfig *m_curItemConfig = nullptr;  // +0x10
};
