#pragma once

#include "db/KQuickInputListBuff.h"
#include "db/KQuickInputStore.h"

#include <map>
#include <string>

// Словари заголовков отчёта (реф. KQuickInputReportTitle1 / KQuickInputReportTitle2 +
// одноимённые DBTableHandler'ы, X-2600). Отдельная ветка от Patient/Doctor/Applicant:
// другие колонки, ДРУГИЕ имена таблиц (С префиксом `tb_`) и другой generic-слой
// (реф. шаблон KDcmDBTableHandler<T> поверх KDcmDBTableHandlerBase::QueryRecords).
//
// Реф. sizeof обеих сущностей — 0x50, layout БАЙТ-ИДЕНТИЧЕН:
//   +0x00 int         Key    (сентинел -1 — «не задан»)
//   +0x08 std::string Title  (сентинел "INVALID_STRING" @0x83df60)
//   +0x28 int         Count  (сентинел -1)
//   +0x30 std::string Time   (в ConvertToMap НЕ участвует, только читается из строки)
// Символа KQuickInputReportTitle1::ConvertToMap в бинарнике нет — тела, судя по всему,
// свёрнуты компоновщиком (ICF) с Title2 (@0x55b180); GetMatchDate обоих хендлеров имеют
// ПОБАЙТОВО совпадающую последовательность инструкций, отличаясь только адресами.
// Поэтому layout и логика вынесены в общую базу, а имена реф.-типов сохранены.
struct KQuickInputReportTitleData
{
    static const char *kInvalidTitle;   // "INVALID_STRING" @0x83df60

    int         Key = -1;
    std::string Title;
    int         Count = -1;
    std::string Time;

    // Реф. @0x55b180: ровно 3 колонки — "Key" @0x83e338, "Title" @0x877ee8,
    // "Count" @0x877ef0. Поля с сентинелами в map НЕ КЛАДУТСЯ (сверено: сравнение с
    // "INVALID_STRING" и `cmn w,#1` уводят в обход записи). "Time" не пишется вовсе.
    std::map<std::string, std::string> ConvertToMap() const;

    // Обратный разбор строки результата. Отдельного символа в реф. нет — заинлайнен в
    // QueryRecordByOtherKey и в обоих GetMatchDate; читает ВСЕ ЧЕТЫРЕ колонки, включая
    // "Time" @0x862678.
    void FromRow(const std::map<std::string, std::string> &row);
};

struct KQuickInputReportTitle1 : KQuickInputReportTitleData {};
struct KQuickInputReportTitle2 : KQuickInputReportTitleData {};

// Общая часть двух хендлеров. Реф. синглтона нет; имя таблицы — обычное поле объекта.
// DEVICE-STUB: реф. generic — KDcmDBTableHandlerBase::QueryRecords(table, spec, rows)
// @0x40d480; здесь тот же контракт через KQuickInputStore (см. db/KQuickInputStore.h).
class KQuickInputReportTitleDBTableHandlerBase
{
public:
    explicit KQuickInputReportTitleDBTableHandlerBase(std::string table)
        : m_table(std::move(table)) {}

    const std::string &TableName() const { return m_table; }

    // Реф. KDcmDBTableHandler<T>::QueryRecordByOtherKey @0x55b890: спека {Where, Limit="1"},
    // условие «field = 'value'», первая строка → все 4 поля сущности.
    // ⚠️ Порядок операндов условия дизасмом до конца НЕ ЗАКРЕПЛЁН; принята естественная
    // трактовка (имя колонки слева) — она согласуется с KDbStrHandler::BuildSimpleCondition.
    bool QueryRecordByOtherKey(const std::string &field, const std::string &value,
                               KQuickInputReportTitleData &out);

    // Реф. GetMatchDate @0x693658 (Title1) / @0x694780 (Title2) — код идентичен:
    //   1) ПРЕДЗАПОЛНЕНИЕ всех 10 слотов _ListBuff: Id="", Name="", Gender=1 (не 2!),
    //      Age=0, DoB = QDate::currentDate().toString("yyyy-MM-dd") (@0x85dc10) —
    //      у заголовков нет даты, это безопасная заглушка;
    //   2) спека {Where: «title like '%<prefix>%'» (@0x88b9a0 + @0x862a50), Limit: "10"};
    //   3) на каждую строку: Name[i] = Title (сверено: цель — массив buff+0x140,
    //      источник — поле +0x8 сущности, шаг 0x50/0x20).
    // Слот Id не пишется ⇒ попап показывает голый заголовок (см. SearchMatchItem).
    bool GetMatchDate(const std::string &prefix, _ListBuff &buff);

    static void SetStore(KQuickInputStore *store) { s_store = store; }
    static KQuickInputStore *Store() { return s_store; }

protected:
    std::string m_table;
    static KQuickInputStore *s_store;
};

// Таблицы — С префиксом `tb_` (реф. литералы @0x862da8 / @0x862de0).
class KQuickInputReportTitle1DBTableHandler : public KQuickInputReportTitleDBTableHandlerBase
{
public:
    KQuickInputReportTitle1DBTableHandler();
};

class KQuickInputReportTitle2DBTableHandler : public KQuickInputReportTitleDBTableHandlerBase
{
public:
    KQuickInputReportTitle2DBTableHandler();
};
