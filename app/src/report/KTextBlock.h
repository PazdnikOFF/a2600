#pragma once

#include "report/KReportTemplateData.h"

#include <QColor>
#include <QFlags>
#include <QString>
#include <Qt>

#include <map>
#include <string>

// Модель текстового блока отчёта (реф. KTextBlock, X-2600). НЕ виджет (нет vtable) —
// обёртка над KReportTemplateItem* + KReportTemplateDataNew*, отдаёт текстовое
// содержимое блока и стилевые атрибуты. Пиксельный рендер — в отдельных потребителях
// (KRTTextItemCreator/KTemplateEditDocument), их реимплементировать не надо.
//
// Данные и стиль берутся из полей KReportTemplateDataNew: m_mapConfigs (@0x00) —
// значения по ключу; m_mapItemConfigs (@0x48) — стиль-конфиги по имени FontType/по ID.
class KTextBlock
{
public:
    KTextBlock(KReportTemplateItem *item, KReportTemplateDataNew *dataNew, bool hideKey = false);

    QString ElementId() const;              // m_pTemplateItem->m_strID
    std::string Data() const;               // значение блока по атрибуту TemplateData, tr()
    std::string Title() const;              // "Заголовок : " (FormatStr "%s : "), если непусто
    QString FullText() const;               // Title + Data

    bool Bold() const;                      // стиль FontType → "Bold" == "1"
    bool Italic() const;                    // стиль FontType → "Italic" == "1"
    // Размер шрифта в pt из стиля (Size). Реф. далее масштабирует pt→px по DPI
    // (physicalDotsPerInch) — device-специфика рендера, здесь опущена (возврат pt).
    int  FontSize(int &out) const;
    bool FontColor(QColor &out) const;                      // FontColor → setNamedColor (всегда true)
    bool Alignment(QFlags<Qt::AlignmentFlag> &out) const;   // AlignH → Left/Center/Right
    bool LineHeight(int n, int &out) const;                 // LineHeight1..5

    KReportTemplateItem *GetTemplateItem() const { return m_pTemplateItem; }
    KReportTemplateItemConfig GetItemConfig() const { return m_itemConfig; }

    void SetHideKey(bool v) { m_bHideKey = v; }
    void SetTextParam(const std::map<std::string, std::string> &p) { m_mapTextParam = p; }

private:
    void InitItemConfig();   // кэширует стиль-конфиг блока по m_strID
    // Значение атрибута item-config по ключу ("" если нет).
    std::string attr(const std::string &key) const;
    // Значение атрибута стиля FontType по ключу ("" если нет).
    std::string styleAttr(const std::string &key) const;

    KReportTemplateItem     *m_pTemplateItem = nullptr;   // +0x00
    KReportTemplateDataNew  *m_pDataNew = nullptr;        // +0x08
    KReportTemplateItemConfig m_itemConfig;               // +0x10
    bool                     m_bHideKey = false;          // +0x68
    std::map<std::string, std::string> m_mapTextParam;    // +0x70
};
