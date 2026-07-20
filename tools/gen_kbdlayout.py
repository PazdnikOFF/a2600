#!/usr/bin/env python3
"""Генератор таблиц раскладок малых языков из бинарника-референса.

Извлекает статические таблицы KSmallLangTranslate из `update/root/X2000`
и печатает готовый C++-заголовок в stdout.

    python3 tools/gen_kbdlayout.py > app/src/sys/KSmallLangTables.h

Таблицы живут В КОДЕ референса (не в конфиг-файлах прошивки), поэтому их
приходится встраивать в наш исходник. Чтобы это не превращалось в ручную
перепечатку 300 строк, файл ГЕНЕРИРУЕТСЯ этим скриптом — результат
воспроизводим байт-в-байт и проверяем.

Адреса получены реверсом (см. docs/PROGRESS.md §10, итерация KSmallLangTranslate).
"""

import os
import struct
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN = os.path.join(REPO, "update", "root", "X2000")

# --- чтение ELF по виртуальному адресу ---------------------------------------

with open(BIN, "rb") as f:
    F = f.read()

_shoff = struct.unpack_from("<Q", F, 0x28)[0]
_shentsize = struct.unpack_from("<H", F, 0x3A)[0]
_shnum = struct.unpack_from("<H", F, 0x3C)[0]
_secs = []
for _i in range(_shnum):
    _o = _shoff + _i * _shentsize
    _type, = struct.unpack_from("<I", F, _o + 4)
    _addr, _off, _size = struct.unpack_from("<QQQ", F, _o + 16)
    _secs.append((_addr, _off, _size, _type))


def rd(va, n):
    """Байты по виртуальному адресу; .bss (SHT_NOBITS=8) читается как нули."""
    for addr, off, size, t in _secs:
        if addr and addr <= va < addr + size:
            if t == 8:
                return b"\0" * n
            return F[off + (va - addr):off + (va - addr) + n]
    raise SystemExit("нет секции для адреса %x" % va)


def i32(b, o):
    return struct.unpack_from("<i", b, o)[0]


def u64(b, o):
    return struct.unpack_from("<Q", b, o)[0]


# --- адреса таблиц (из реверса) ----------------------------------------------

MOD_BIT_CONF = 0xA4DFE0          # 96 B, 8 x 12
IDX_ARRAYS = {                   # каждая 0x40 = 16 x int
    0xA4E040: "Fn",
    0xA4E080: "AltGr",
    0xA4E0C0: "LftChar",
    0xA4E100: "Mark",
    0xA4E140: "HungaryAlpha",
    0xA4E180: "PolishAlpha",
    0xA4E1C0: "FrenchAlpha",
    0xA4E200: "Alpha",
}
LATIN_CMB_CHAR = 0xA4DE28        # 440 B, 11 x 40
KBD_DETAIL = [                   # каждая 2080 B, 52 x 40
    ("Latin", 0xA50540),
    ("Russian", 0xA4FD20),       # реф. имя — g_astRussianKbdDetail_Endo
    ("French", 0xA4F500),
    ("Polish", 0xA4ECE0),
    ("Hungary", 0xA4E4C0),
]

out = sys.stdout.write

out("""// СГЕНЕРИРОВАНО `python3 tools/gen_kbdlayout.py` — НЕ РЕДАКТИРОВАТЬ ВРУЧНУЮ.
//
// Статические таблицы раскладок малых языков, извлечённые из бинарника-референса
// update/root/X2000 (реф. KSmallLangTranslate,
// platform/language/SmallLanguage/KSmallLangTranslate.cpp).
//
// Эти данные лежат В КОДЕ оригинала, а не в конфигах прошивки, поэтому их
// приходится встраивать в исходник. Генерация скриптом (а не перепечатка)
// гарантирует побайтовое совпадение с бинарником и повторяемость.
#pragma once

#include "sys/KSmallLangTypes.h"

namespace smalllang {

""")

# --- g_astModBitConf_S50 -----------------------------------------------------

d = rd(MOD_BIT_CONF, 0x60)
out("// {eKey, nBitIdx, nOutIdxBit}; терминатор — нулевой eKey.\n")
out("// Биты nModBitState: 0=Shift_L 1=Shift_R 2=CapsLock 3=AltGr 4=Fn 5=Ctrl 6=Alt.\n")
out("// nOutIdxBit == -1 (Ctrl, Alt) ⇒ в индекс уровня НЕ участвуют.\n")
out("inline const stModBitConf g_astModBitConf_S50[8] = {\n")
for i in range(8):
    out("    { 0x%03x, %d, %d },\n" % (i32(d, i * 12), i32(d, i * 12 + 4), i32(d, i * 12 + 8)))
out("};\n\n")

# --- индексные массивы уровней ------------------------------------------------

out("// modIdx (4 бита: 0=Shift 1=CapsLock 2=AltGr 3=Fn) -> индекс уровня в aeOutKey.\n")
out("// -1 означает «клавиша проглатывается» (ProcMultiLevelKey обнуляет keysym).\n")
for a, n in IDX_ARRAYS.items():
    d = rd(a, 0x40)
    vals = ", ".join("%d" % i32(d, j * 4) for j in range(16))
    out("inline const int g_aiModCmb2OutKeyArrIdxFor%s[16] = { %s };\n" % (n, vals))
out("\n")

# --- g_astLatinDetail_CmbChar ------------------------------------------------

d = rd(LATIN_CMB_CHAR, 0x1B8)
out("// Таблица «мёртвых клавиш» латиницы: {eBaseKey, aeOut[8], pad}.\n")
out("inline const stCmbCharDetail g_astLatinDetail_CmbChar[11] = {\n")
for i in range(11):
    o = i * 40
    outs = ", ".join("0x%03x" % i32(d, o + 4 + j * 4) for j in range(8))
    out("    { 0x%03x, { %s }, %d },\n" % (i32(d, o), outs, i32(d, o + 36)))
out("};\n\n")

out("""// ВАЖНО (реверс): g_astFrenchDetail_CmbChar и g_astPolishDetail_CmbChar лежат
// в .bss и ПОЛНОСТЬЮ НУЛЕВЫЕ — статического инициализатора нет, во время работы
// их никто не заполняет. То есть французские/польские мёртвые клавиши защёлкивают
// префикс, но ProcCombStepKey совпадения не находит НИКОГДА и всегда уходит
// в ветку переигрывания трёх событий. Воспроизводим как честно пустые.
inline const stCmbCharDetail g_astFrenchDetail_CmbChar[1] = { { 0, { 0 }, 0 } };
inline const stCmbCharDetail g_astPolishDetail_CmbChar[1] = { { 0, { 0 }, 0 } };

""")

# --- g_ast*KbdDetail ---------------------------------------------------------

out("// Основные таблицы клавиш: {eKey, aeOutKey[7], piModCmb2OutKeyArrIdx}.\n")
out("// Терминатор — нулевой eKey (52-я запись).\n")
for name, a in KBD_DETAIL:
    d = rd(a, 0x820)
    out("inline const stKbdDetail g_ast%sKbdDetail[52] = {\n" % name)
    for i in range(52):
        o = i * 40
        p = u64(d, o + 32)
        outs = ", ".join("0x%03x" % i32(d, o + 4 + j * 4) for j in range(7))
        idx = ("g_aiModCmb2OutKeyArrIdxFor" + IDX_ARRAYS[p]) if p in IDX_ARRAYS else "nullptr"
        out("    { 0x%03x, { %s }, %s },\n" % (i32(d, o), outs, idx))
    out("};\n\n")

out("} // namespace smalllang\n")
