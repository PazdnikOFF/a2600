#pragma once

#include <string>
#include <vector>

// Класс эндоскопа/осмотра (реф. KScopeClass::E_CLASS, X-2600).
//
// ⚠️ ВНИМАНИЕ, КОЛЛИЗИЯ: в нашем `report/KThesaurusOpt.h` есть СВОЙ enum
// ScopeClass с ДРУГИМ порядком (Duodeno=2). У референса Duodeno == 14.
// Смешивать их НЕЛЬЗЯ — молча перепутаются Bronchoscope и Duodenoscope.
// Здесь заведён отдельный тип с ЗНАЧЕНИЯМИ РЕФЕРЕНСА (подтверждены разбором
// цепочки cmp в GetReportTypeName: cbz→0, cmp #1/#2/#3/#0xe).
namespace KScopeClass {

enum E_CLASS {
    E_GASTROSCOPY     = 0,
    E_COLONOSCOPY     = 1,
    E_BRONCHOSCOPE    = 2,
    E_NOSELARYNXSCOPE = 3,
    E_DUODENOSCOPE    = 14,   // именно 14, не 4
};

// Имена устройств (реф. std::vector<std::string> в .bss, 6 записей по 32 байта).
// ⚠️ РАСХОЖДЕНИЕ РЕФЕРЕНСА: Get/SetUserOrganNameList пропускают значения 0..14
// (`cmp w0,#0xe; b.ls`), но индексируют этот 6-элементный вектор как enum<<5 ⇒
// при E_DUODENOSCOPE(14) выходят ЗА ГРАНИЦЫ. Здесь индекс приводится к 0..5
// (14 → 4, DEV_DUODENOSCOPE) — иначе воспроизводить пришлось бы UB.
const char *DeviceName(E_CLASS cls);

} // namespace KScopeClass

// Типы курсора-указателя на схеме органа (реф. exam_detail::E_CURSOR_TYPE).
namespace exam_detail {
enum E_CURSOR_TYPE {
    E_CURSOR_NONE            = 0,
    E_ARROW_RIGHT_DOWN_CURSOR = 1,
    E_ARROW_RIGHT_UP_CURSOR   = 2,
    E_ARROW_LEFT_UP_CURSOR    = 3,
    E_ARROW_LEFT_DOWN_CURSOR  = 4,
    E_POINT_CURSOR            = 5,
};
// Реф. константа: базовый каталог ресурсов qss.
extern const char *const CONFIG_QSS_BASE_DIR;   // "mainapp/application/qss/"
} // namespace exam_detail

namespace report_edit {
extern const char *const POS_NAME_DIR;     // "patient/posname/"
extern const char *const REGION_DIR;       // "mainapp/patient/region/"
extern const char *const EDITED_MARK_IMG;  // "mainapp/application/qss/patient/edited_mark.png"
} // namespace report_edit
