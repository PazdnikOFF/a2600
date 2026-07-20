#!/usr/bin/env python3
"""Генератор таблиц имён клавиш (KKey2Name) из бинарника-референса.

Извлекает из `update/root/X2000` все таблицы «код клавиши -> имя», которые
референс собирает КОДОМ (а не хранит готовыми в .rodata), и печатает готовый
C++-заголовок в stdout:

    python3 tools/gen_keysym.py > app/src/sys/KKeyNameTables.h

Что именно извлекается (см. docs/PROGRESS.md, итерация KKey2Name):

  * `KKey2Name::GetNameOfKey(int)` @0x776808 (0x862c байт!) — НЕ читает
    g_strKeysym_S40/S50, а КАЖДЫЙ ВЫЗОВ собирает на стеке (24608 байт кадра)
    массив из 614 записей `{ int eKey; std::string strName; }` (шаг 40 байт),
    линейно ищет по eKey и возвращает копию имени; при промахе — "SONO_Unknown"
    (литерал .rodata @0x89e5d0; путь промаха — 0x77d0d4, проверено дизасмом).
    Это и есть настоящая таблица keysym -> имя.

  * `KKey2Name::GetNameOfQtScancode(int)` @0x77ee38 — 54 записи в .bss
    @0x14e6ba0, тот же layout `{int; std::string}`, заполняется статическим
    инициализатором; при промахе — "".

  * `g_strKeysym_S40` @0x14e7410 (233 x std::string) и
    `g_strKeysym_S50` @0x14e9130 (357 x std::string) — просто массивы имён,
    БЕЗ кодов; индекс = позиция в массиве. Заполняются тем же статическим
    инициализатором `_GLOBAL__sub_I_KKey2Name.cpp` @0x2b0ac8 (0x5868 байт).
    ВАЖНО: во всём бинарнике их никто НЕ ЧИТАЕТ (см. проверку ниже) —
    мёртвые данные, оставлены для справки.

Метод извлечения: мини-эмулятор AArch64 (adrp/add/sub/mov*/ldr/str/bl) гонится
линейно по телу функции, накапливая (адрес -> 32-битное значение) для `str w`
и (адрес -> строковый литерал) для вызовов конструкторов std::string. Затем
записи склеиваются по правилу «строка лежит на 8 байт выше своего кода».
Ветвления игнорируются: обе функции — прямолинейные цепочки инициализации.
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
_shstrndx = struct.unpack_from("<H", F, 0x3E)[0]
_secs = []
for _i in range(_shnum):
    _o = _shoff + _i * _shentsize
    _nameoff, _type = struct.unpack_from("<II", F, _o)
    _addr, _off, _size = struct.unpack_from("<QQQ", F, _o + 16)
    _link, _info = struct.unpack_from("<II", F, _o + 40)
    _entsize = struct.unpack_from("<Q", F, _o + 56)[0]
    _secs.append([_nameoff, _type, _addr, _off, _size, _link, _entsize, ""])
_stro = _secs[_shstrndx][3]
for _s in _secs:
    _e = F.index(b"\0", _stro + _s[0])
    _s[7] = F[_stro + _s[0]:_e].decode()
SEC = {s[7]: s for s in _secs}


def rd(va, n):
    """Байты по виртуальному адресу; .bss (SHT_NOBITS=8) читается как нули."""
    for _n, t, addr, off, size, _l, _e, _nm in _secs:
        if addr and addr <= va < addr + size:
            if t == 8:
                return b"\0" * n
            return F[off + (va - addr):off + (va - addr) + n]
    raise SystemExit("нет секции для адреса %x" % va)


def in_sec(va, name):
    s = SEC.get(name)
    return s is not None and s[2] <= va < s[2] + s[4]


def cstr(va):
    b = rd(va, 256)
    return b[:b.index(b"\0")].decode("utf-8", "replace")


# --- таблица символов ---------------------------------------------------------

def _symbols():
    st = SEC[".symtab"]
    strt = _secs[st[5]][3]
    out = {}
    for i in range(st[4] // 24):
        o = st[3] + i * 24
        nm, = struct.unpack_from("<I", F, o)
        val, size = struct.unpack_from("<QQ", F, o + 8)
        e = F.index(b"\0", strt + nm)
        out[F[strt + nm:strt + nm + (e - strt - nm)].decode()] = (val, size)
    return out


SYM = _symbols()

# --- R_AARCH64_RELATIVE: чем заполняются слоты GOT ----------------------------

GOT = {}
for _sn in (".rela.dyn", ".rela.plt"):
    _s = SEC.get(_sn)
    if not _s:
        continue
    for _i in range(_s[4] // 24):
        _o = _s[3] + _i * 24
        _off, _info, _add = struct.unpack_from("<QQq", F, _o)
        if (_info & 0xFFFFFFFF) == 1027:          # R_AARCH64_RELATIVE
            GOT[_off] = _add

# --- мини-эмулятор AArch64 ----------------------------------------------------

# конструкторы std::string, у которых x1 — указатель на строковый литерал
STR_CTORS = {
    SYM["_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC2EPKcRKS3_.isra.19"][0],
    SYM["_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC2EPKcRKS3_.isra.31"][0],
    SYM["_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE12_M_constructIPKcEEvT_S8_St20forward_iterator_tag"][0],
}

SP = 31          # индекс регистра SP в нашем файле регистров
FRAME = 0x7F000000   # искусственная база стека


def _sx(v, bits):
    return v - (1 << bits) if v & (1 << (bits - 1)) else v


def emulate(start, size):
    """Линейно исполняет функцию, возвращает (ints, strs).

    ints: {адрес: 32-битное значение}  — из `str w<n>, [x<m>, #imm]`
    strs: {адрес: VA строкового литерала} — из вызовов конструкторов std::string
    """
    r = [None] * 32
    r[SP] = FRAME
    ints, strs = {}, {}
    code = rd(start, size)
    for pc in range(start, start + size, 4):
        w, = struct.unpack_from("<I", code, pc - start)
        rd_, rn, rm = w & 31, (w >> 5) & 31, (w >> 16) & 31

        if (w & 0x9F000000) == 0x90000000:                       # ADRP
            imm = ((((w >> 5) & 0x7FFFF) << 2) | ((w >> 29) & 3))
            r[rd_] = (pc & ~0xFFF) + (_sx(imm, 21) << 12)
        elif (w & 0x7F800000) in (0x11000000, 0x51000000):       # ADD/SUB immediate
            v = (w >> 10) & 0xFFF
            if (w >> 22) & 1:
                v <<= 12
            base = r[rn]
            if base is None:
                r[rd_] = None
            else:
                r[rd_] = base - v if (w & 0x40000000) else base + v
        elif (w & 0xFF200000) in (0x8B000000, 0xCB000000):       # ADD/SUB shifted reg 64
            a = r[rn] if rn != 31 or True else 0
            b = r[rm]
            if a is None or b is None:
                r[rd_] = None
            else:
                r[rd_] = a + b if (w & 0xFF200000) == 0x8B000000 else a - b
        elif (w & 0x7F800000) in (0x52800000, 0xD2800000):       # MOVZ
            r[rd_] = ((w >> 5) & 0xFFFF) << (((w >> 21) & 3) * 16)
        elif (w & 0x7F800000) in (0x12800000, 0x92800000):       # MOVN
            r[rd_] = ~(((w >> 5) & 0xFFFF) << (((w >> 21) & 3) * 16)) & 0xFFFFFFFFFFFFFFFF
        elif (w & 0x7F800000) in (0x72800000, 0xF2800000):       # MOVK
            sh = ((w >> 21) & 3) * 16
            if r[rd_] is not None:
                r[rd_] = (r[rd_] & ~(0xFFFF << sh)) | (((w >> 5) & 0xFFFF) << sh)
        elif (w & 0xFF200000) in (0xAA000000, 0x2A000000) and rn == 31:   # MOV reg
            r[rd_] = 0 if rm == 31 else r[rm]
        elif (w & 0xFFC00000) == 0xF9400000:                     # LDR x, [xn, #imm]
            if r[rn] is None:
                r[rd_] = None
            else:
                a = r[rn] + (((w >> 10) & 0xFFF) * 8)
                r[rd_] = GOT.get(a)                              # слот GOT -> адрес объекта
        elif (w & 0xFFC00000) == 0xB9000000:                     # STR w, [xn, #imm]
            if r[rn] is not None:
                v = 0 if rd_ == 31 else r[rd_]
                if v is not None:
                    ints[r[rn] + (((w >> 10) & 0xFFF) * 4)] = v & 0xFFFFFFFF
        elif (w & 0xFC000000) == 0x94000000:                     # BL
            tgt = pc + (_sx(w & 0x3FFFFFF, 26) << 2)
            if tgt in STR_CTORS and r[0] is not None and r[1] is not None:
                if in_sec(r[1], ".rodata"):
                    strs[r[0]] = r[1]
            r[0] = r[1] = r[2] = r[3] = r[4] = None              # регистры-аргументы затёрты
            for i in range(5, 19):
                r[i] = None
    return ints, strs


def pairs(ints, strs):
    """Склеивает {int eKey; std::string} — строка лежит на +8 от кода."""
    out = []
    for a in sorted(ints):
        if a + 8 in strs:
            out.append((ints[a], cstr(strs[a + 8])))
    return out


# --- 1. GetNameOfKey: 614 записей, собираемых на стеке ------------------------

_gnok_va, _gnok_sz = SYM["_ZN9KKey2Name12GetNameOfKeyB5cxx11Ei"]
KEYS = pairs(*emulate(_gnok_va, _gnok_sz))

# --- 2. статический инициализатор: QtScancode + g_strKeysym_S40/S50 -----------

_init_va, _init_sz = SYM["_GLOBAL__sub_I_KKey2Name.cpp"]
_ii, _is = emulate(_init_va, _init_sz)

_qt_va = SYM["_ZN9KKey2Name19GetNameOfQtScancodeB5cxx11Ei"][0]
SCAN = [(k, v) for (k, v) in pairs(_ii, _is)]

S40_VA, S40_SZ = SYM["_Z15g_strKeysym_S40B5cxx11"]
S50_VA, S50_SZ = SYM["_Z15g_strKeysym_S50B5cxx11"]


def arr(va, size):
    n = size // 32
    return [cstr(_is[va + 32 * i]) if va + 32 * i in _is else None for i in range(n)]


S40 = arr(S40_VA, S40_SZ)
S50 = arr(S50_VA, S50_SZ)

# --- вывод ---------------------------------------------------------------------

out = sys.stdout.write

out("""// СГЕНЕРИРОВАНО `python3 tools/gen_keysym.py` — НЕ РЕДАКТИРОВАТЬ ВРУЧНУЮ.
//
// Имена клавиш (E_SONO_KEY -> строка), извлечённые из бинарника-референса
// update/root/X2000, класс KKey2Name (platform/.../KKey2Name.cpp).
//
// В оригинале KKey2Name::GetNameOfKey КАЖДЫЙ ВЫЗОВ пересобирает эту таблицу
// на стеке (кадр 24 КБ, 614 записей) и линейно ищет по ней. Мы храним её как
// статические данные; поведение при промахе воспроизведено ("SONO_Unknown").
#pragma once

#include <string>

namespace keyname {

struct stKeyName { int eKey; const char* pszName; };

""")

out("// %d записей; порядок — как в референсе (важен: возможны дубли кодов,\n"
    "// линейный поиск возвращает ПЕРВОЕ совпадение).\n" % len(KEYS))
out("inline const stKeyName g_astKeyName[%d] = {\n" % len(KEYS))
for k, v in KEYS:
    out('    { 0x%04x, "%s" },\n' % (k, v.replace("\\", "\\\\").replace('"', '\\"')))
out("};\n\n")

out("// KKey2Name::GetNameOfKey: линейный поиск, при промахе — \"SONO_Unknown\"\n"
    "// (.rodata @0x89e5d0, ветка 0x77d0d4).\n"
    "inline std::string GetNameOfKey(int eKey)\n{\n"
    "    for (const stKeyName& st : g_astKeyName)\n"
    "        if (st.eKey == eKey) return st.pszName;\n"
    "    return \"SONO_Unknown\";\n}\n\n")

out("// %d записей: Qt-скан-код -> имя (реф. .bss @0x%x).\n" % (len(SCAN), 0x14E6BA0))
out("inline const stKeyName g_astQtScancodeName[%d] = {\n" % len(SCAN))
for k, v in SCAN:
    out('    { 0x%04x, "%s" },\n' % (k, v.replace("\\", "\\\\").replace('"', '\\"')))
out("};\n\n")

out("// KKey2Name::GetNameOfQtScancode: при промахе — пустая строка.\n"
    "inline std::string GetNameOfQtScancode(int nScancode)\n{\n"
    "    for (const stKeyName& st : g_astQtScancodeName)\n"
    "        if (st.eKey == nScancode) return st.pszName;\n"
    "    return \"\";\n}\n\n")

for nm, a, va in (("S40", S40, S40_VA), ("S50", S50, S50_VA)):
    out("// g_strKeysym_%s @0x%x — %d имён БЕЗ кодов (индекс = позиция).\n"
        "// В бинарнике-референсе НИКТО эти массивы не читает — мёртвые данные.\n"
        % (nm, va, len(a)))
    out("inline const char* const g_apszKeysym_%s[%d] = {\n" % (nm, len(a)))
    for s in a:
        out('    %s,\n' % ('nullptr' if s is None else '"%s"' % s.replace("\\", "\\\\").replace('"', '\\"')))
    out("};\n\n")

out("} // namespace keyname\n")
