#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "report/KReportTemplateData.h"

class KRTSimpleCreatorContext;

// Базовый «творец» блока в Simple-движке (реф. KRTSimpleAbsItemCreator, ctor @0x50a420).
// Реф. базовая реализация `CreateItem` @0x50a3f8 — `assert(false)`, т.е. метод обязателен
// к переопределению; в порте — чисто виртуальный (тот же приём, что с KRTAbsDataSource).
//
// ⚠️ У шести конкретных творцов СОБСТВЕННЫХ ПОЛЕЙ НЕТ (проверено по их ctor): всё
// состояние — через ссылку на контекст (реф. `this+8`).
//
// ⚠️ КЛЮЧЕВОЕ ПОНИМАНИЕ ДВИЖКА: результаты `Get*Data` реф. НИГДЕ НЕ СОХРАНЯЕТ — буфер
// заполняется и тут же освобождается. Simple-движок не собирает контент, он вычисляет
// ДОСТУПНОСТЬ элементов: при успехе зовёт `KReportDisplayParam::AppendValidItem(m_strID)`.
// Это же объясняет, почему `IsItemValid` читает ДРУГОЕ множество — набор «валидных»
// наполняется здесь для ПОТРЕБИТЕЛЯ (второго движка отображения), а не для себя.
class KRTSimpleAbsItemCreator
{
public:
    explicit KRTSimpleAbsItemCreator(KRTSimpleCreatorContext &ctx) : m_ctx(ctx) {}
    virtual ~KRTSimpleAbsItemCreator() = default;

    // Реф. vtable-слот 2 (+0x10). Возвращает int — и это НЕ всегда 0/1: табличный и
    // sub-data творцы возвращают СУММУ по под-элементам.
    virtual int CreateItem(const KReportTemplateItem &item,
                           const std::map<std::string, std::string> &params) = 0;

protected:
    // Общий пролог всех шести: если params непуст, поле прогоняется через
    // report_template::ConvertToSourceID(поле, params, out); иначе берётся как есть.
    static std::string Resolve(const std::string &field,
                               const std::map<std::string, std::string> &params);
    // Общий эпилог: пометить элемент валидным в KReportDisplayParam контекста.
    void AppendValid(const std::string &id) const;

    KRTSimpleCreatorContext &m_ctx;
};

// Журнал вызовов — НАШЕ дополнение (в реф. его нет), чтобы поведение движка было
// проверяемо self-test'ом без реального рендера.
struct CreatedRecord {
    std::string creator;   // имя класса-творца
    std::string itemId;    // KReportTemplateItem::m_strID
    std::map<std::string, std::string> params;
};
namespace rt_simple_log {
const std::vector<CreatedRecord> &Records();
void Clear();
void Append(const std::string &creator, const std::string &itemId,
            const std::map<std::string, std::string> &params);
}   // namespace rt_simple_log

// Реф. @0x50b408. Пустой m_strDataSrc → БЕЗ обращения к источнику: сразу валиден и 1.
class KRTSimpleTextItemCreator : public KRTSimpleAbsItemCreator
{
public:
    using KRTSimpleAbsItemCreator::KRTSimpleAbsItemCreator;
    int CreateItem(const KReportTemplateItem &item,
                   const std::map<std::string, std::string> &params) override;
};

// Реф. @0x50b158. Слот 5 GetTextGroupData. Пустой m_strDataSrc → 0 (в отличие от Text!).
class KRTSimpleTextGroupCreator : public KRTSimpleAbsItemCreator
{
public:
    using KRTSimpleAbsItemCreator::KRTSimpleAbsItemCreator;
    int CreateItem(const KReportTemplateItem &item,
                   const std::map<std::string, std::string> &params) override;
};

// Реф. @0x50a758. Сначала берёт ПОСЛЕДНИЙ токен m_strID (разделитель '/', иммедиата
// 0x2f) и ищет его в статической карте локальных картинок MAP_LOCAL_PIC_PFINFO
// (@0xa78f98). Нашёл → валиден и 1 БЕЗ обращения к источнику. Иначе слот 6 GetImageData.
class KRTSimpleImageItemCreator : public KRTSimpleAbsItemCreator
{
public:
    using KRTSimpleAbsItemCreator::KRTSimpleAbsItemCreator;
    int CreateItem(const KReportTemplateItem &item,
                   const std::map<std::string, std::string> &params) override;

    // DEVICE-STUB: содержимое MAP_LOCAL_PIC_PFINFO из бинарника НЕ восстановлено
    // (статическая карта, заполняется в рантайме) — вынесено в инъектируемый набор.
    static void SetLocalPicNames(const std::set<std::string> &names);
    static const std::set<std::string> &LocalPicNames();
};

// Реф. @0x50a4a8. Слот 7 GetImageGroupData. Пустой m_strDataSrc → 0.
class KRTSimpleImageGroupCreator : public KRTSimpleAbsItemCreator
{
public:
    using KRTSimpleAbsItemCreator::KRTSimpleAbsItemCreator;
    int CreateItem(const KReportTemplateItem &item,
                   const std::map<std::string, std::string> &params) override;
};

// Реф. @0x50af00. К источнику данных НЕ обращается ВООБЩЕ: рекурсивно прогоняет
// m_lstSubItems через KRTSimpleCreatorContext::CreateItem и СУММИРУЕТ результаты.
class KRTSimpleTableItemCreator : public KRTSimpleAbsItemCreator
{
public:
    using KRTSimpleAbsItemCreator::KRTSimpleAbsItemCreator;
    int CreateItem(const KReportTemplateItem &item,
                   const std::map<std::string, std::string> &params) override;
};

// Реф. @0x50ac20. Слот 8 GetSubData(m_strDataSrc, localData) — обратите внимание, id
// берётся из m_strDataSrc, НЕ из m_strID. При успехе переносит конфиги элементов
// в KReportDisplayParam через AppendItemParam и рекурсивно строит вложенные элементы.
class KRTSimpleSubDataItemCreator : public KRTSimpleAbsItemCreator
{
public:
    using KRTSimpleAbsItemCreator::KRTSimpleAbsItemCreator;
    int CreateItem(const KReportTemplateItem &item,
                   const std::map<std::string, std::string> &params) override;
};
