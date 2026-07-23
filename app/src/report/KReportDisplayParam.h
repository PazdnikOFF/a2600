#pragma once

#include <QString>
#include <QSet>
#include <QVector>

#include <map>
#include <set>
#include <string>

#include "report/KReportTemplateData.h"   // KReportTemplateItemConfig

struct ReportItem;
class KReportDataSource;

// Параметры отображения элементов отчёта (реф. KReportDisplayParam, X-2600).
// Держит множество «валидных» (отображаемых) элементов и решает, показывать ли
// элемент шаблона. Валидность выводится из наличия данных в источнике
// (реф. UpdateTemplateDisplayParam + IsItemValid/AppendValidItem/SetRefValidItems/Reset).
//
// Модель: имя элемента шаблона (ReportItem::name) валидно, если у него есть
// непустое связанное значение в KReportDataSource (текст/таблица — по DataSrc;
// изображение — по пути снимка), либо есть хотя бы один валидный потомок.
// Опциональное «эталонное» множество (SetRefValidItems) ограничивает, какие имена
// вообще допустимы (реф.: AppendValidItem гейтит по ref-множеству).
class KReportDisplayParam
{
public:
    void Reset();                                          // очистить валидные

    // Эталонное множество допустимых имён (реф. SetRefValidItems). Пустое = без ограничения.
    void SetRefValidItems(const QSet<QString> &ref) { ref_ = ref; }

    // Добавить элемент в валидные (реф. AppendValidItem). Если задано ref-множество и
    // имя в него не входит — игнорируется. true, если добавлен/уже присутствует.
    bool AppendValidItem(const QString &name);

    bool IsItemValid(const QString &name) const;          // реф. IsItemValid
    int  ValidCount() const { return valid_.size(); }
    QSet<QString> ValidItems() const { return valid_; }

    // Пройти дерево шаблона + источник данных, пометить валидные элементы.
    // ⚠️ ЭТО НАША МОДЕЛЬ, А НЕ РЕФЕРЕНС (см. блок ниже): рекурсивный обход шаблона
    // придуман нами. Возвращает число валидных.
    int UpdateTemplateDisplayParam(const QVector<ReportItem> &items,
                                   const KReportDataSource &ds);

    // ══════════════════════ РЕФЕРЕНСНЫЙ СЛОЙ (ROADMAP C8 снят) ══════════════════════
    // Коллизия разрешена СОВМЕЩЕНИЕМ: реф. методы имеют ТЕ ЖЕ ИМЕНА, отличаясь только
    // типами аргументов (std::string/set/map вместо QString/QSet/QVector) — поэтому они
    // добавлены перегрузками в этот же класс, а прежний упрощённый слой сохранён.
    //
    // ⚠️ ВАЖНО: реф. класс — это ПАРАМЕТР-КОНТЕЙНЕР, а не вычислитель. Дизасм показал,
    // что `UpdateTemplateDisplayParam` @0x507110 состоит РОВНО из двух `operator=`
    // (никакого разбора ключей, никаких разделителей, набор валидных не трогается),
    // а `AppendValidItem` @0x506ca0 НЕ гейтится по ref-множеству и возвращает void.
    // Наш упрощённый слой выше моделирует другое поведение — оба сосуществуют осознанно.
    //
    // Реф. layout (sizeof 0xD0): флаг +0x08, map<string,string> +0x10,
    // map<string,ItemConfig> +0x40, set<string> валидных +0x70, set<string> ref +0xa0.

    // Реф. @0x506ca0: безусловная вставка в набор валидных. Гейта НЕТ, возврата НЕТ.
    void AppendValidItem(const std::string &name);

    // Реф. @0x506a38 — АСИММЕТРИЧНЫЙ краевой случай, легко потерять при «упрощении»:
    //   ref-множество НИ РАЗУ не задавалось (флаг +0x08 == 0) → ВСЕГДА true;
    //   задано, но пустое                                     → ВСЕГДА false.
    // Проверяется именно РЕФЕРЕНСНОЕ множество (+0xa0), а НЕ то, что наполняет
    // AppendValidItem (+0x70) — эти два набора через IsItemValid не связаны.
    bool IsItemValid(const std::string &name) const;

    // Реф. @0x506ec0: присваивание + взвод флага.
    void SetRefValidItems(const std::set<std::string> &items);

    // Реф. @0x506ef0: MERGE-вставка (существующие ключи НЕ перезаписываются).
    void AppendItemParam(const std::map<std::string, KReportTemplateItemConfig> &params);

    // Реф. @0x507110: ровно два `operator=` — конфиг шаблона и конфиги элементов
    // ЗАМЕЩАЮТСЯ целиком (в отличие от merge-семантики AppendItemParam).
    void UpdateTemplateDisplayParam(
        const std::map<std::string, std::string> &configs,
        const std::map<std::string, KReportTemplateItemConfig> &itemConfigs);

    const std::map<std::string, std::string> &TemplateConfigs() const { return m_mapTemplateConfig; }
    const std::map<std::string, KReportTemplateItemConfig> &ItemConfigs() const { return m_mapItemConfig; }
    const std::set<std::string> &RefValidItems() const { return m_setRefValidItems; }
    const std::set<std::string> &ValidItemsStd() const { return m_setValidItems; }
    bool HasRefValidItems() const { return m_bHasRefValidItems; }

private:
    // Рекурсивная проверка одного элемента: валиден ли он сам или его поддерево.
    bool markItem(const ReportItem &it, const KReportDataSource &ds);
    QSet<QString> valid_;
    QSet<QString> ref_;

    // Реф. поля (имена наши — символов полей в бинарнике нет).
    bool                                            m_bHasRefValidItems = false;  // +0x08
    std::map<std::string, std::string>              m_mapTemplateConfig;          // +0x10
    std::map<std::string, KReportTemplateItemConfig> m_mapItemConfig;             // +0x40
    std::set<std::string>                           m_setValidItems;              // +0x70
    std::set<std::string>                           m_setRefValidItems;           // +0xa0
};
