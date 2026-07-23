#pragma once

#include <map>
#include <string>
#include <vector>

#include "report/KReportTemplateData.h"

class KRTSimpleCreatorContext;

// Базовый «творец» блока в Simple-движке (реф. KRTSimpleAbsItemCreator).
// Реф. базовая реализация `CreateItem` @0x50a3f8 — `assert(false)` (литерал "true == false",
// файл .../simpledisplay/KRTSimpleAbsItemCreator.cpp), т.е. метод обязателен к переопределению.
// В порте это выражено чисто виртуальным методом — как мы уже сделали для KRTAbsDataSource.
class KRTSimpleAbsItemCreator
{
public:
    explicit KRTSimpleAbsItemCreator(KRTSimpleCreatorContext &ctx) : m_ctx(ctx) {}
    virtual ~KRTSimpleAbsItemCreator() = default;

    // Реф. vtable-слот 2 (+0x10). Возвращает int, трактуемый как «успех».
    virtual int CreateItem(const KReportTemplateItem &item,
                           const std::map<std::string, std::string> &params) = 0;

protected:
    KRTSimpleCreatorContext &m_ctx;
};

// Шесть конкретных творцов (имена реф., из символов). Их ТЕЛА в этой итерации НЕ
// реверсированы — здесь они ЗАПИСЫВАЮЩИЕ ЗАГЛУШКИ: фиксируют (тип, id, params), чтобы
// поведение самого движка (выбор творца и раскрутка повторов) было проверяемо.
// Это честная граница: движок портирован по дизасму, наполнение блоков — нет.
struct CreatedRecord {
    std::string creator;   // имя класса-творца
    std::string itemId;    // KReportTemplateItem::m_strID
    std::map<std::string, std::string> params;
};

class KRTSimpleRecordingCreator : public KRTSimpleAbsItemCreator
{
public:
    KRTSimpleRecordingCreator(KRTSimpleCreatorContext &ctx, std::string name)
        : KRTSimpleAbsItemCreator(ctx), m_name(std::move(name)) {}

    int CreateItem(const KReportTemplateItem &item,
                   const std::map<std::string, std::string> &params) override;

    static const std::vector<CreatedRecord> &Records();
    static void ClearRecords();

private:
    std::string m_name;
};

#define K_RT_SIMPLE_CREATOR(Cls, Label)                                            \
    class Cls : public KRTSimpleRecordingCreator                                   \
    {                                                                              \
    public:                                                                        \
        explicit Cls(KRTSimpleCreatorContext &ctx)                                 \
            : KRTSimpleRecordingCreator(ctx, Label) {}                             \
    };

K_RT_SIMPLE_CREATOR(KRTSimpleTextItemCreator,    "KRTSimpleTextItemCreator")
K_RT_SIMPLE_CREATOR(KRTSimpleTextGroupCreator,   "KRTSimpleTextGroupCreator")
K_RT_SIMPLE_CREATOR(KRTSimpleImageItemCreator,   "KRTSimpleImageItemCreator")
K_RT_SIMPLE_CREATOR(KRTSimpleImageGroupCreator,  "KRTSimpleImageGroupCreator")
K_RT_SIMPLE_CREATOR(KRTSimpleTableItemCreator,   "KRTSimpleTableItemCreator")
K_RT_SIMPLE_CREATOR(KRTSimpleSubDataItemCreator, "KRTSimpleSubDataItemCreator")

#undef K_RT_SIMPLE_CREATOR
