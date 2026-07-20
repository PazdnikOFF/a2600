#!/usr/bin/env python3
"""Генератор таблицы диспетчеризации KLcdProxy::m_lcdActTab из бинарника-референса.

В отличие от gen_endoscope.py / gen_camera.py (там таблицы СТРОЯТСЯ КОДОМ и нужна
эмуляция AArch64), здесь таблица лежит ГОТОВОЙ в секции .data по адресу символа
`_ZN9KLcdProxy11m_lcdActTabE` @0xa4cc88, размер 0x7b0 = 1968 байт.

Раскладка элемента (24 байта) — восстановлена по значениям и по Itanium C++ ABI:

    struct _LcdActItem {                 // sizeof = 24, alignof = 8
        quint16 usKey;                   // +0x00  код клавиши/источника
        quint16 usAct;                   // +0x02  тип события (KeyEventAct
                                         //        сравнивает ТОЛЬКО младший байт):
                                         //          2 = press (remote/foot, без длит.)
                                         //          3 = short press
                                         //          4 = long press
                                         //          7 = set value (запись настройки)
        /* +0x04  4 байта padding */
        int (KLcdProxy::*pfnAct)(int);   // +0x08  указатель на член, 16 байт:
                                         //        +0x08 ptr  (адрес кода; бит0=0,
                                         //                    т.е. НЕ виртуальный)
                                         //        +0x10 adj  (this-adjustment, =0)
    };

Указатель-на-член в Itanium ABI — это пара {ptr, adj}; поскольку все обработчики
невиртуальные и база одна (QObject), ptr = обычный адрес функции, adj = 0. Именно
поэтому элемент 24, а не 16 байт, и поэтому вторые 8 байт всегда нулевые.

Запуск:
    python3 tools/gen_lcdproxy.py            # человекочитаемый дамп
    python3 tools/gen_lcdproxy.py --header   # C++-заголовок в stdout
    python3 tools/gen_lcdproxy.py --csv      # CSV (key,act,symbol,addr)
"""

import os
import struct
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN = os.path.join(REPO, "update", "root", "X2000")
NM = "/usr/bin/nm"

TAB_VA = 0xA4CC88          # _ZN9KLcdProxy11m_lcdActTabE
TAB_SIZE = 0x7B0           # 1968
ITEM_SIZE = 24
ITEM_COUNT = TAB_SIZE // ITEM_SIZE   # 82

# ---- чтение ELF по виртуальному адресу (тот же приём, что в gen_endoscope.py) ----

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


# ---- таблица символов: адрес -> имя (для резолва указателей на члены) ----

def load_symbols():
    out = subprocess.check_output([NM, BIN], stderr=subprocess.DEVNULL).decode(
        "utf-8", "replace")
    syms = {}
    for line in out.splitlines():
        parts = line.split()
        if len(parts) < 3:
            continue
        addr, typ, name = parts[0], parts[1], parts[2]
        if typ not in ("T", "t", "W", "w"):
            continue
        try:
            a = int(addr, 16)
        except ValueError:
            continue
        # localalias-дубли не должны вытеснять основное имя
        if a in syms and ".localalias" in name:
            continue
        syms.setdefault(a, name)
        if ".localalias" not in name:
            syms[a] = name
    return syms


SYMS = load_symbols()


def demangle_klcdproxy(mangled):
    """Мини-демангл для _ZN9KLcdProxy<len><Name>E<args> (macOS c++filt не умеет)."""
    p = "_ZN9KLcdProxy"
    if not mangled.startswith(p):
        return mangled
    rest = mangled[len(p):]
    i = 0
    while i < len(rest) and rest[i].isdigit():
        i += 1
    if i == 0:
        return mangled
    n = int(rest[:i])
    name = rest[i:i + n]
    args = rest[i + n:]
    argmap = {"Ei": "(int)", "Ev": "()", "Eii": "(int,int)", "Et": "(ushort)"}
    return "KLcdProxy::" + name + argmap.get(args, "(" + args + ")")


# ---- разбор таблицы ----

ACT_NAME = {2: "PRESS", 3: "SHORT", 4: "LONG", 7: "SETVAL"}


def parse_table():
    raw = read_va(TAB_VA, TAB_SIZE)
    assert len(raw) == TAB_SIZE, "не удалось прочитать m_lcdActTab"
    items = []
    for i in range(ITEM_COUNT):
        o = i * ITEM_SIZE
        key, act = struct.unpack_from("<HH", raw, o)
        pad = struct.unpack_from("<I", raw, o + 4)[0]
        ptr, adj = struct.unpack_from("<QQ", raw, o + 8)
        sym = SYMS.get(ptr, "?")
        items.append(dict(idx=i, key=key, act=act, pad=pad,
                          ptr=ptr, adj=adj, sym=sym,
                          name=demangle_klcdproxy(sym)))
    return items


def main():
    items = parse_table()

    # --- инварианты, которые подтверждают раскладку ---
    assert all(it["pad"] == 0 for it in items), "padding не нулевой -> раскладка иная"
    assert all(it["adj"] == 0 for it in items), "this-adj != 0 -> множественное наследование"
    assert all(it["ptr"] & 1 == 0 for it in items), "бит0 ptr=1 -> виртуальный обработчик"
    unresolved = [it for it in items if it["sym"] == "?"]
    dup = {}
    for it in items:
        dup.setdefault((it["key"], it["act"]), []).append(it["idx"])
    collisions = {k: v for k, v in dup.items() if len(v) > 1}

    if "--csv" in sys.argv:
        print("idx,key,act,act_name,addr,symbol")
        for it in items:
            print("%d,%d,%d,%s,0x%x,%s" % (
                it["idx"], it["key"], it["act"],
                ACT_NAME.get(it["act"], "?"), it["ptr"], it["sym"]))
        return

    if "--header" in sys.argv:
        print("// Сгенерировано tools/gen_lcdproxy.py — НЕ РЕДАКТИРОВАТЬ ВРУЧНУЮ.")
        print("// Источник: KLcdProxy::m_lcdActTab @0x%x, %d байт, %d записей.\n"
              % (TAB_VA, TAB_SIZE, ITEM_COUNT))
        print("struct _LcdActItem {")
        print("    quint16 usKey;")
        print("    quint16 usAct;              // 2=press 3=short 4=long 7=set-value")
        print("    int (KLcdProxy::*pfnAct)(int);")
        print("};\n")
        print("static const _LcdActItem m_lcdActTab[%d] = {" % ITEM_COUNT)
        for it in items:
            print("    { %3d, %d, &KLcdProxy::%s }," % (
                it["key"], it["act"],
                it["name"].split("::")[-1].split("(")[0]
                if it["sym"] != "?" else "/*0x%x*/" % it["ptr"]))
        print("};")
        return

    # --- человекочитаемый дамп ---
    print("KLcdProxy::m_lcdActTab @0x%x  size=%d  item=%d B  count=%d"
          % (TAB_VA, TAB_SIZE, ITEM_SIZE, ITEM_COUNT))
    print("проверки: pad==0 OK, this-adj==0 OK, ptr бит0==0 (невиртуальные) OK")
    print("нерезолвленных указателей: %d ; коллизий (key,act): %d"
          % (len(unresolved), len(collisions)))
    keys = sorted({it["key"] for it in items})
    print("уникальных key: %d (диапазон %d..%d)" % (len(keys), keys[0], keys[-1]))
    acts = sorted({it["act"] for it in items})
    print("значения act: %s" % ", ".join(
        "%d(%s)" % (a, ACT_NAME.get(a, "?")) for a in acts))
    print()
    print("%3s %4s %4s %-6s %-10s %s" % ("#", "key", "act", "type", "addr", "handler"))
    print("-" * 78)
    for it in items:
        print("%3d %4d %4d %-6s 0x%-8x %s" % (
            it["idx"], it["key"], it["act"], ACT_NAME.get(it["act"], "?"),
            it["ptr"], it["name"]))

    # какие клавиши имеют и short, и long
    both = sorted(k for k in keys
                  if {3, 4} <= {it["act"] for it in items if it["key"] == k})
    print("\nклавиши с обоими событиями (short+long): %s" %
          (", ".join(map(str, both)) or "нет"))
    only_long = sorted(k for k in keys
                       if {it["act"] for it in items if it["key"] == k} == {4})
    print("только LONG: %s" % (", ".join(map(str, only_long)) or "нет"))
    only_short = sorted(k for k in keys
                        if {it["act"] for it in items if it["key"] == k} == {3})
    print("только SHORT: %s" % (", ".join(map(str, only_short)) or "нет"))


if __name__ == "__main__":
    main()
