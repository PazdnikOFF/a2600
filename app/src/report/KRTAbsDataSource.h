#pragma once

#include "KReportTemplateData.h"

#include <string>
#include <vector>
#include <utility>

// Абстрактный источник данных report-template движка (реф. KRTAbsDataSource, base @0x5892a0,
// vtable _ZTV16KRTAbsDataSource @0xa210c8, 12 слотов). Инъектируемый SEAM: «Simple»-движок
// (KRTSimpleDisplay/CreatorContext) тянет значения полей отчёта ЧЕРЕЗ этот интерфейс. Реф. Get*
// в базе — assert(false) (порт делает их чисто-виртуальными → форсирует override); GetTextOptional/
// GetMix2Data → false; UpdateDataSource/CacheData — no-op. SplitDataSrcID — НЕвиртуальный хелпер
// (реф. @0x589768, через report_template::RevertPathByID). Все Get*(id, out&) заполняют out и
// возвращают true, если id разрешён. Порт: чистый STL, без Qt/device. Реализация — KRTDataSourceStub.
class KRTAbsDataSource
{
public:
    virtual ~KRTAbsDataSource() = default;                                     // слот 0/1

    virtual void UpdateDataSource() {}                                         // слот 2: no-op
    virtual bool GetTextData(const std::string &id, std::string &out) = 0;     // слот 3
    virtual bool GetTextOptional(const std::string &fieldId, const std::string &key,
                                 std::vector<std::string> &out) { (void)fieldId; (void)key; (void)out; return false; }   // слот 4
    virtual bool GetTextGroupData(const std::string &id,
                                  std::vector<std::pair<std::string, std::string>> &out) = 0;   // слот 5
    virtual bool GetImageData(const std::string &id, std::string &out) = 0;    // слот 6
    virtual bool GetImageGroupData(const std::string &id,
                                   std::vector<std::pair<std::string, std::string>> &out) = 0;  // слот 7
    virtual bool GetSubData(const std::string &id, KReportTemplateDataNew &out) = 0;            // слот 8
    virtual bool GetMixData(const std::string &id, std::vector<std::string> &out) = 0;          // слот 9
    virtual bool GetMix2Data(const std::string &id, std::vector<std::vector<std::string>> &out) // слот 10
    { (void)id; (void)out; return false; }
    virtual void CacheData() {}                                                // слот 11

    // Реф. @0x589768 (НЕвиртуальный): split id по разделителю → две половины. true при успехе.
    bool SplitDataSrcID(const std::string &id, std::string &outA, std::string &outB) const;
};
