#pragma once

#include <list>
#include <map>
#include <string>

// Центральные структуры данных отчётной подсистемы (реф. X-2600).
// На них завязаны KTemplateCfg, KTemplateLibCfg, KTextBlock/KImageBlock/KTableBlock,
// report_template::*. Имена ПОЛЕЙ в оригинале невосстановимы (нет DWARF для кода
// приложения) — смещения/типы взяты из дизасма, имена наши.

// Элемент дерева <Content> (реф. KReportTemplateItem, sizeof=0xF8).
// ВСЕ поля — строки (реф. xml_attribute::as_string("")): отсутствующий атрибут даёт
// ПУСТУЮ СТРОКУ, а не 0/false. Column/ShowTitle тоже строки — конверсия в числа
// происходит позже, у потребителей (KTableBlock/KTextBlock/KImageBlock).
struct KReportTemplateItem {
    std::string m_strID;         // +0x00 — вычисляемое: parentPath + "/" + Name
    std::string m_strName;       // +0x20 — Name
    std::string m_strTitle;      // +0x40 — Title (ключ перевода TR_*)
    std::string m_strType;       // +0x60 — Type (RT_TEXT_BLOCK/RT_TABLE_BLOCK/…)
    std::string m_strDataSrc;    // +0x80 — DataSrc ("<источник>,<ключ поля>")
    std::string m_strColumn;     // +0xa0 — Column
    std::string m_strShowTitle;  // +0xc0 — ShowTitle
    std::list<KReportTemplateItem> m_lstSubItems;   // +0xe0 — дети (рекурсия <Content>)
};

// Параметры узла/стиля из <ItemConfig> (реф. KReportTemplateItemConfig, sizeof=0x58).
// ВАЖНО: реф. НЕ разбирает Section/AlignH/FontType/ColumnRatio/SplitLine*/… в
// отдельные типизированные поля — он кладёт ВСЕ атрибуты узла как есть в generic-map
// строк. Ни int, ни bool, ни split по запятой на этом уровне нет: "25,50,25" остаётся
// строкой, а разбирают её потребители (report_template::GetSplitLineInfo и т.п.).
// Единственное типизированное поле — UserDefine.
struct KReportTemplateItemConfig {
    bool m_bUserDefine = false;                     // +0x00 — UserDefine == "1"
    std::string m_strName;                          // +0x08 — Name (= ключ внешней map)
    std::map<std::string, std::string> m_mapAttrs;  // +0x28 — ВСЕ атрибуты узла (вкл. Name)
};

// Полезная нагрузка одного шаблона (реф. KReportTemplateDataNew).
// Ключ m_mapItemConfigs — атрибут Name узла <ItemConfig>/<Item> КАК ЕСТЬ, без
// нормализации. В одной map лежат две разновидности (различие — соглашение
// потребителя, не парсера): пути ("/", "/RT_HEADER/…") — параметры конкретного узла
// Content; именованные стили ("FirstTitle"/"SecondTitle"/…) — шрифтовые пресеты,
// на которые ссылается атрибут FontType. Путь начинается с '/', стиль — нет.
struct KReportTemplateDataNew {
    std::map<std::string, std::string> m_mapConfigs;   // +0x00 — <TemplateConfig> Name→Value
    std::list<KReportTemplateItem> m_lstItems;         // +0x30 — дерево <Content>
    std::map<std::string, KReportTemplateItemConfig> m_mapItemConfigs;  // +0x48 — <ItemConfig>
};

// Параметры разделительной линии секции (реф. KSplitLineInfo, sizeof 0x50). Наполняется
// report_template::GetSplitLineInfo из атрибутов item-config. Тип/цвет — обычные строки
// (реф. НЕ QColor/QTextLength; десериализуются потребителем при рендере).
struct KSplitLineInfo {
    int m_nSplitLineWidth = 0;         // +0x00 — "SplitLineWidth" (обязателен: gate)
    int m_nSplitLineSpace = 0;         // +0x04 — "SplitLineSpace"
    int m_nSplitStartIndex = 0;        // +0x08 — "SplitStartIndex"
    std::string m_strSplitLineType;    // +0x10 — "SplitLineType", дефолт "Horizontal"
    std::string m_strSplitLineColor;   // +0x30 — "SplitLineColor", дефолт "black"
};
