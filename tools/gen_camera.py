#!/usr/bin/env python3
"""Генератор статических таблиц класса KCamera из бинарника-референса.

Извлекает из `update/root/X2000` таблицу, которую референс собирает КОДОМ
(а не хранит готовой в .rodata):

  * `KCamera::InitCameraSeriesMap()` @0x6b3dc8 (0x50c байт) — строит ОДНУ
    карту `QMap<QString,QString>` («модель камеры» -> «код серии»),
    доступную через `GetCameraSeriesMap()`.

    В отличие от KEndoScope::InitEndoSeriesMap (там ДВЕ карты — основная и
    «уханьская»), здесь карта ровно одна: в теле функции один
    QMapDataBase::createData и одна пара QMap<QString,QString>::~QMap в
    unwind-пути.

    Пары кладутся на стек как массив QString [k0,v0,k1,v1,...] с шагом 8 байт
    (std::pair<QString,QString> = 16 байт), затем вставляются
    inline-развёрнутым QMap::insert.

    Количество записей проверяется независимо: 14 вызовов
    QString::fromAscii_helper == 14 строк == 7 пар.

Метод извлечения тот же, что в tools/gen_endoscope.py и tools/gen_keysym.py:
мини-эмулятор AArch64 (adrp / add / mov / str) гонится линейно по телу
функции. Вызов `QString::fromAscii_helper(const char*, int)` даёт
(адрес литерала, длина); последующий `str x0, [sp, #off]` привязывает литерал
к слоту кадра. Ветвления игнорируются: тело — прямолинейная цепочка
инициализации.

Машинерия (чтение ELF по VA, дизассемблер, эмулятор) переиспользуется из
tools/gen_endoscope.py — здесь только цель и рендеринг.

Запуск:
    python3 tools/gen_camera.py            # человекочитаемый дамп
    python3 tools/gen_camera.py --header   # C++-заголовок в stdout
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from gen_endoscope import extract_slots  # noqa: E402  (общая машинерия)

# --- цели ---------------------------------------------------------------------

INIT_CAMERA_SERIES_MAP = (0x6B3DC8, 0x50C)

# Ожидаемое число пар, посчитанное независимо по вызовам
# QString::fromAscii_helper в теле функции (14 строк == 7 пар).
EXPECTED_PAIRS = 7


def series_pairs():
    """-> список (model, series) для GetCameraSeriesMap()."""
    slots = extract_slots(*INIT_CAMERA_SERIES_MAP)
    # Слоты идут строго по возрастанию смещения в кадре: [k0,v0,k1,v1,...].
    # Разрыв монотонности означал бы вторую карту (как в KEndoScope) — для
    # KCamera такого быть не должно, поэтому это assert, а не разделитель.
    prev = -1
    for _a, off, _s in slots:
        if off <= prev:
            raise SystemExit(
                f"gen_camera: разрыв монотонности смещений кадра на 0x{_a:x} "
                f"(off=0x{off:x} <= prev=0x{prev:x}) — похоже, карт больше одной; "
                f"см. group-логику в gen_endoscope.series_pairs()")
        prev = off
    if len(slots) % 2:
        raise SystemExit(f"gen_camera: нечётное число строк ({len(slots)})")
    pairs = [(slots[i][2], slots[i + 1][2]) for i in range(0, len(slots), 2)]
    if len(pairs) != EXPECTED_PAIRS:
        raise SystemExit(
            f"gen_camera: извлечено {len(pairs)} пар, ожидалось {EXPECTED_PAIRS} "
            f"(по числу вызовов QString::fromAscii_helper)")
    return pairs


def main():
    pairs = series_pairs()

    if "--header" in sys.argv:
        print("#pragma once")
        print("// СГЕНЕРИРОВАНО tools/gen_camera.py — НЕ РЕДАКТИРОВАТЬ.")
        print("// Источник: KCamera::InitCameraSeriesMap() @0x6b3dc8 в update/root/X2000.")
        print()
        print("#include <QMap>")
        print("#include <QString>")
        print()
        print("// Модель камеры -> код серии. Отдаётся через "
              "KCamera::GetCameraSeriesMap().")
        print("static const QMap<QString, QString> kCameraSeriesMap = {")
        for k, v in pairs:
            print(f'    {{ QStringLiteral("{k}"), QStringLiteral("{v}") }},')
        print("};")
        return

    print(f"# InitCameraSeriesMap: {len(pairs)} pairs (одна карта)")
    print("\n## GetCameraSeriesMap()  (QMap<QString,QString>)")
    for k, v in pairs:
        print(f"{k:<16} -> {v}")


if __name__ == "__main__":
    main()
