#pragma once

#include <cstdint>

// Типы подсистемы раскладок «малых языков» (реф. X-2600,
// platform/language/SmallLanguage/KSmallLangTranslate.cpp).
//
// ⚠️ Символических ИМЁН клавиш в бинарнике нет: их выдаёт только
// KKey2Name::GetNameOfKey, читающий массивы g_strKeysym_S40/S50 в .bss
// (233 / 357 std::string, заполняются _GLOBAL__sub_I). Пока используем
// сырые коды; расшифровка массивов имён — отдельная задача.
using E_SONO_KEY = int;

namespace smalllang {

// Коды клавиш-модификаторов (из g_astModBitConf_S50 и веток кода).
enum : E_SONO_KEY {
    KEY_CAPSLOCK = 0x449,
    KEY_SHIFT_L  = 0x456,
    KEY_SHIFT_R  = 0x462,
    KEY_CTRL     = 0x463,
    KEY_FN       = 0x464,
    KEY_ALT      = 0x466,
    KEY_ALTGR    = 0x468,
};

// {клавиша, номер бита в nModBitState, номер бита в индексе уровня (-1 — не участвует)}
struct stModBitConf {
    E_SONO_KEY eKey;
    int        nBitIdx;
    int        nOutIdxBit;
};

// {клавиша, до 7 вариантов вывода по уровням, таблица modIdx→уровень}
struct stKbdDetail {
    E_SONO_KEY  eKey;
    E_SONO_KEY  aeOutKey[7];
    const int  *piModCmb2OutKeyArrIdx;
};

// «Мёртвая клавиша»: {базовая клавиша, вывод по индексу префикса, выравнивание}
struct stCmbCharDetail {
    E_SONO_KEY eBaseKey;
    E_SONO_KEY aeOut[8];
    int        pad;
};

// Реф. stKbdLayout, sizeof 0x80.
struct stKbdLayout {
    int                     nModBitState;         // 0x00 битовая карта модификаторов
    E_SONO_KEY              aeBaseModifier[8];    // 0x04 0-терминированный
    E_SONO_KEY              aeLockedModifier[8];  // 0x24 0-терминированный
    E_SONO_KEY              aeCombPrefixKey[8];   // 0x44 НЕ терминируется: всегда 0..7
    E_SONO_KEY              ePendingCombPrefix;   // 0x64 защёлка мёртвой клавиши
    const stCmbCharDetail  *pCmbChar;             // 0x68
    const stModBitConf     *pModBitConf;          // 0x70 всегда g_astModBitConf_S50
    const stKbdDetail      *pKbdDetail;           // 0x78
};

// Событие клавиатуры панели (реф. stPADKeyboardEvent, шаг 20 байт).
struct stPADKeyboardEvent {
    uint8_t    bRelease;    // 0x00
    E_SONO_KEY eKey;        // 0x08
    int        nModState;   // 0x0c
};

} // namespace smalllang
