#pragma once

#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "report/KRTSimpleAbsItemCreator.h"

class KRTAbsDataSource;
class KReportDisplayParam;

// Контекст Simple-движка отчёта (реф. KRTSimpleCreatorContext, ctor @0x508b50).
// Держит реестр творцов по типу блока и раскручивает «повторы» элементов.
//
// ⚠️ Он НЕ владеет KReportDisplayParam — получает указатель на объект, встроенный
// ПО ЗНАЧЕНИЮ в KRTSimpleDisplay (+0x10). Владение: KRTSimpleDisplay → context.
class KRTSimpleCreatorContext
{
public:
    KRTSimpleCreatorContext(KRTAbsDataSource *pDataSource, KReportDisplayParam *pDisplayParam);
    virtual ~KRTSimpleCreatorContext();

    KRTSimpleCreatorContext(const KRTSimpleCreatorContext &) = delete;
    KRTSimpleCreatorContext &operator=(const KRTSimpleCreatorContext &) = delete;

    // Реф. @0x508160: ДЕВЯТЬ пар «ключ → творец» (у «взрослого» KRTCreatorContext
    // @0x547918 их шесть). Отличия Simple-версии:
    //   • появляется класс `KRTSimpleTextGroupCreator` — аналога во «взрослом» НЕТ;
    //   • `KRTSimpleTableItemCreator` регистрируется под ЧЕТЫРЬМЯ ключами (добавлены
    //     RT_ROW_TABLE_BLOCK и RT_OB_Z_SCORE_BLOCK), причём это ЧЕТЫРЕ ОТДЕЛЬНЫХ
    //     объекта (4 разных `operator new`), а не один общий указатель.
    void InitCreator();

    // Реф. @0x509350. Сам НИЧЕГО не создаёт — готовит «измерения повтора»:
    //  1) ищет item.m_strID в item-конфигах KReportDisplayParam; нет — сразу HandleRepeat
    //     с ПУСТЫМ списком;
    //  2) в найденном конфиге ищет ключ "RepeatCommand"; нет — то же;
    //  3) есть → report_template::RevertPathByID(значение, "RefColumnID");
    //  4) по атрибутам того же конфига: ключи, которых ЕЩЁ НЕТ во входящем params,
    //     разрешаются через источник данных, и пара (ключ, варианты) кладётся в список;
    //  5) HandleRepeat(item, params, список).
    int CreateItem(const KReportTemplateItem &item,
                   const std::map<std::string, std::string> &params);

    // Реф. @0x508c00 — РЕКУРСИЯ, дающая ДЕКАРТОВО ПРОИЗВЕДЕНИЕ вариантов:
    //   список пуст → найти творца по item.m_strType и вызвать его CreateItem(item, params);
    //   иначе → снять ПЕРВУЮ пару (ключ, варианты), и для КАЖДОГО варианта положить
    //           params[ключ] = вариант и рекурсивно вызвать себя с остатком списка,
    //           СУММИРУЯ результаты.
    // При N ключах с M1…Mn вариантами творец вызывается M1×…×Mn раз.
    int HandleRepeat(const KReportTemplateItem &item,
                     const std::map<std::string, std::string> &params,
                     const std::list<std::pair<std::string, std::vector<std::string>>> &repeats);

    KRTAbsDataSource *DataSource() const { return m_pDataSource; }
    KReportDisplayParam *DisplayParam() const { return m_pDisplayParam; }

private:
    KRTAbsDataSource    *m_pDataSource = nullptr;     // +0x08 (не владеет)
    KReportDisplayParam *m_pDisplayParam = nullptr;   // (не владеет — см. комментарий выше)
    std::map<std::string, std::unique_ptr<KRTSimpleAbsItemCreator>> m_mapCreators;
};
