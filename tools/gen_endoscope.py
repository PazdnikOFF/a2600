#!/usr/bin/env python3
"""Генератор статических таблиц класса KEndoScope из бинарника-референса.

Извлекает из `update/root/X2000` таблицы, которые референс собирает КОДОМ
(а не хранит готовыми в .rodata):

  * `KEndoScope::InitEndoSeriesMap()` @0x6b94a8 (0x1118 байт) — строит ДВЕ
    карты `QMap<QString,QString>` («модель эндоскопа» -> «код серии»):
    основную и «уханьскую» (доступны через GetEndoSeriesMap() /
    GetEndoSeriesMapWuHan()). Пары кладутся на стек как массив QString
    [k0,v0,k1,v1,...] с шагом 8 байт (std::pair<QString,QString> = 16 байт),
    затем вставляются inline-развёрнутым QMap::insert.

  * `KEndoScope::IsSuperfineEndo(QString)`  @0x6b8de0 — QStringList из 6 имён
  * `KEndoScope::IsEndoHasChannel(QString)` @0x6b9108 — QStringList из 4 имён
    Обе строят список на стеке и возвращают QStringList::contains(...,
    Qt::CaseSensitive) НАПРЯМУЮ: членство в списке == true.

Метод извлечения тот же, что в tools/gen_keysym.py: мини-эмулятор AArch64
(adrp / add / mov / str) гонится линейно по телу функции. Вызов
`QString::fromAscii_helper(const char*, int)` даёт (адрес литерала, длина);
последующий `str x0, [sp, #off]` привязывает литерал к слоту кадра.
Ветвления игнорируются: тела — прямолинейные цепочки инициализации.

Запуск:
    python3 tools/gen_endoscope.py            # человекочитаемый дамп
    python3 tools/gen_endoscope.py --header   # C++-заголовок в stdout
"""

import os
import re
import struct
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN = os.path.join(REPO, "update", "root", "X2000")
OBJDUMP = "/usr/bin/objdump"

# --- чтение ELF по виртуальному адресу (тот же приём, что в gen_keysym.py) ----

with open(BIN, "rb") as f:
    F = f.read()

_shoff = struct.unpack_from("<Q", F, 0x28)[0]
_shentsize = struct.unpack_from("<H", F, 0x3A)[0]
_shnum = struct.unpack_from("<H", F, 0x3C)[0]
_SECS = []
for _i in range(_shnum):
    _o = _shoff + _i * _shentsize
    _addr, _off, _size = struct.unpack_from("<QQQ", F, _o + 16)
    _SECS.append((_addr, _off, _size))


def read_va(va, n):
    for a, off, sz in _SECS:
        if a and a <= va < a + sz:
            return F[off + va - a: off + va - a + n]
    return b""


def cstr(va, maxlen=256):
    return read_va(va, maxlen).split(b"\0")[0].decode("utf-8", "replace")


# --- дизассемблер участка -----------------------------------------------------

INSN = re.compile(r"^\s*([0-9a-f]+):\s+[0-9a-f]{8}\s+(\S+)\s*(.*)$")


def disasm(start, end):
    out = subprocess.run(
        [OBJDUMP, "-d", f"--start-address={hex(start)}", f"--stop-address={hex(end)}", BIN],
        capture_output=True, text=True).stdout
    res = []
    for line in out.splitlines():
        m = INSN.match(line)
        if m:
            res.append((int(m.group(1), 16), m.group(2), m.group(3).strip()))
    return res


FROM_ASCII = "_ZN7QString16fromAscii_helperEPKci"


def _callee_saved(reg):
    m = re.fullmatch(r"[xw](\d+)", reg)
    return bool(m) and 19 <= int(m.group(1)) <= 28


def extract_slots(start, size):
    """Эмуляция: -> список (адрес_str_инструкции, смещение_в_кадре, строка)."""
    regs = {}
    slots = []
    for _addr, op, args in disasm(start, start + size):
        a = [x.strip() for x in args.split("//")[0].split(",")]
        if op == "adrp":
            regs[a[0]] = int(a[1].split("<")[0].strip(), 16)
        elif op == "add" and len(a) == 3 and a[2].startswith("#"):
            if a[1] in regs:
                regs[a[0]] = regs[a[1]] + int(a[2][1:].split()[0], 16)
            else:
                regs.pop(a[0], None)
        elif op == "mov" and len(a) == 2:
            if a[1].startswith("#"):
                regs[a[0]] = int(a[1][1:].split()[0], 16)
            elif a[1] in regs:
                regs[a[0]] = regs[a[1]]
            else:
                regs.pop(a[0], None)
        elif op == "bl":
            # AAPCS64: x0-x18 — caller-saved (затираются вызовом), x19-x28 —
            # callee-saved (переживают вызов). Компилятор держит adrp-базы
            # литералов именно в x19-x28, поэтому их НЕЛЬЗЯ сбрасывать —
            # иначе теряются строки, собранные после вызовов QMap::insert.
            ptr, ln = regs.get("x0"), regs.get("w1")
            regs = {k: v for k, v in regs.items() if _callee_saved(k)}
            if FROM_ASCII in args and isinstance(ptr, int) and isinstance(ln, int):
                regs["x0"] = ("STR", ptr, ln)
        elif op == "str" and len(a) >= 2 and a[0] == "x0":
            v = regs.get("x0")
            if isinstance(v, tuple) and v[0] == "STR":
                m = re.search(r"\[sp,?\s*#?(0x[0-9a-f]+|\d+)?\]?", args)
                off = int(m.group(1), 16) if (m and m.group(1)) else 0
                s = read_va(v[1], v[2]).decode("utf-8", "replace")
                slots.append((_addr, off, s))
                # Значение "потребляется" первым же str: компилятор иногда
                # дублирует тот же x0 во второй (scratch) слот кадра — такие
                # повторы не являются новыми элементами массива.
                regs.pop("x0", None)
    return slots


# --- цели ---------------------------------------------------------------------

INIT_ENDO_SERIES_MAP = (0x6B94A8, 0x1118)
IS_SUPERFINE_ENDO = (0x6B8DE0, 0x1AC)
IS_ENDO_HAS_CHANNEL = (0x6B9108, 0x164)


def series_pairs():
    """-> (main_pairs, wuhan_pairs) как списки (model, series)."""
    slots = extract_slots(*INIT_ENDO_SERIES_MAP)
    # Слоты идут строго по возрастанию смещения в кадре: [k0,v0,k1,v1,...].
    # Границу между двумя картами ищем по разрыву в монотонности смещений.
    groups, cur, prev = [], [], -1
    for _a, off, s in slots:
        if off <= prev:
            groups.append(cur)
            cur = []
        cur.append((off, s))
        prev = off
    groups.append(cur)
    res = []
    for g in groups:
        res.append([(g[i][1], g[i + 1][1]) for i in range(0, len(g) - 1, 2)])
    while len(res) < 2:
        res.append([])
    return res[0], res[1]


def stringlist(target):
    return [s for _a, _o, s in extract_slots(*target)]


def main():
    main_map, wuhan_map = series_pairs()
    sup = stringlist(IS_SUPERFINE_ENDO)
    chan = stringlist(IS_ENDO_HAS_CHANNEL)

    if "--header" in sys.argv:
        print("#pragma once\n// СГЕНЕРИРОВАНО tools/gen_endoscope.py — НЕ РЕДАКТИРОВАТЬ.\n")
        print("#include <QMap>\n#include <QString>\n#include <QStringList>\n")
        for name, pairs in (("kEndoSeriesMap", main_map),
                            ("kEndoSeriesMapWuHan", wuhan_map)):
            print(f"static const QMap<QString, QString> {name} = {{")
            for k, v in pairs:
                print(f'    {{ QStringLiteral("{k}"), QStringLiteral("{v}") }},')
            print("};\n")
        for name, lst in (("kSuperfineEndoList", sup), ("kEndoHasChannelList", chan)):
            print(f"static const QStringList {name} = {{"
                  + ", ".join(f'QStringLiteral("{s}")' for s in lst) + " };")
        return

    print(f"# InitEndoSeriesMap: main={len(main_map)} pairs, wuhan={len(wuhan_map)} pairs")
    print("\n## GetEndoSeriesMap()  (QMap<QString,QString>)")
    for k, v in main_map:
        print(f"{k:<16} -> {v}")
    print("\n## GetEndoSeriesMapWuHan()  (QMap<QString,QString>)")
    for k, v in wuhan_map:
        print(f"{k:<16} -> {v}")
    print(f"\n## IsSuperfineEndo list ({len(sup)}): {sup}")
    print(f"## IsEndoHasChannel list ({len(chan)}): {chan}")


if __name__ == "__main__":
    main()
