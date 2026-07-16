#pragma once

#include "report/KReportTemplateData.h"

#include <QString>

#include <map>
#include <string>

// Модель блока-картинки отчёта (реф. KImageBlock, X-2600). Прямой сиблинг KTextBlock:
// НЕ виджет (нет vtable), обёртка над KReportTemplateItem* + KReportTemplateDataNew*.
// Пиксельный рендер — в отдельных потребителях (KRTImageItemCreator и т.п.).
//
// Отличие от KTextBlock: НЕТ m_bHideKey; Width/Heigth/GetAlign читают item-config-
// атрибуты (ImageWidth/ImageHeight/AlignH); Url резолвит путь картинки по DataSrc.
class KImageBlock
{
public:
    KImageBlock(KReportTemplateItem *item, KReportTemplateDataNew *dataNew);

    QString ElementId() const;                 // m_pTemplateItem->m_strID
    QString ImageName() const;                 // tr(m_pTemplateItem->m_strName) — от Name узла!
    // Путь к файлу картинки; valid = загрузился ли файл как QImage. Путь возвращается
    // ВСЕГДА (даже если невалиден). Резолв: DataSrc → SplitStr(",") → last → PicMap[key].
    QString Url(bool &valid) const;
    int Width() const;                         // ImageWidth (ConvertStringToInt), -1 если нет
    int Heigth() const;                        // ImageHeight (sic — опечатка реф.), -1 если нет
    std::string GetAlign() const;              // AlignH как есть ("" если нет)

    KReportTemplateItem *GetTemplateItem() const { return m_pTemplateItem; }
    KReportTemplateItemConfig GetItemConfig() const { return m_itemConfig; }
    void SetTextParam(const std::map<std::string, std::string> &p) { m_mapTextParam = p; }

    // Глобальный кэш source-id → локальный путь картинки (реф. MAP_LOCAL_PIC_PFINFO,
    // .bss; наполняется в рантайме потребителями типа KRTDataSourceReal::GetImageData).
    // Здесь — статическая карта с регистрацией (для off-device/тестов).
    static void RegisterPicPath(const std::string &sourceId, const std::string &path);
    static void ClearPicMap();

private:
    void InitItemConfig();
    std::string attr(const std::string &key) const;

    KReportTemplateItem     *m_pTemplateItem = nullptr;   // +0x00
    KReportTemplateDataNew  *m_pDataNew = nullptr;        // +0x08
    KReportTemplateItemConfig m_itemConfig;               // +0x10
    std::map<std::string, std::string> m_mapTextParam;    // +0x68
};
