#!/bin/bash
# Обёртка над hermes (Debian x86_64) — среда реверса с полным GNU-тулчейном.
# Настроена 2026-07-20 (см. docs/PROGRESS.md §11). Референс update/root/*
# лежит на hermes в ~/a2600ref/bin; деманглированные карты — локально в
# tools/symmaps/ (в .gitignore: полный API-срез проприетарного SonoScape).
#
# Использование:
#   tools/rev.sh sym  <regex>            — греп по деманглированным символам (ЛОКАЛЬНО, мгновенно)
#   tools/rev.sh dis  <bin> <start> <stop> — чистый aarch64-дизасм диапазона (на hermes)
#   tools/rev.sh dec  <bin> <func|addr>  — Ghidra-декомпиляция функции в C (на hermes)
#   tools/rev.sh run  <bin> [args...]    — запуск standalone-бинарника под qemu-aarch64
#   tools/rev.sh filt                    — деманглер (stdin → stdout), рабочий c++filt
#
# <bin> ∈ X2000 | X2000Monitor | X2000Simulator | X2000Video
set -euo pipefail
HOST=hermes

# Предохранитель: hermes делит ресурсы с продовыми контейнерами пользователя. Перед
# ЛЮБОЙ тяжёлой операцией проверяем, что хосту есть чем дышать; иначе — отказ, а не
# «попробуем и посмотрим». Пороги: >=1500 МиБ available и loadavg(1m) < 3.0.
preflight() {
    local info avail load
    info=$(ssh -o ConnectTimeout=15 "$HOST" \
        'free -m | awk "/^Mem:/{print \$7}"; cut -d" " -f1 /proc/loadavg' 2>/dev/null) || {
        echo "rev.sh: $HOST недоступен — операция отменена" >&2; exit 1; }
    avail=$(printf '%s\n' "$info" | sed -n 1p)
    load=$(printf '%s\n' "$info" | sed -n 2p)
    if [ "${avail:-0}" -lt 1500 ]; then
        echo "rev.sh: на $HOST всего ${avail} МиБ available (<1500) — отказ, чтобы не уронить хост" >&2
        exit 1
    fi
    if awk -v l="${load:-0}" 'BEGIN{exit !(l+0 >= 3.0)}'; then
        echo "rev.sh: loadavg на $HOST = ${load} (>=3.0) — отказ, хост уже занят" >&2
        exit 1
    fi
}
REF='~/a2600ref'
HERE="$(cd "$(dirname "$0")/.." && pwd)"
cmd="${1:-}"; shift || true

case "$cmd" in
  sym)
    grep -iE "${1:?regex}" "$HERE"/tools/symmaps/*.symmap ;;
  dis)
    bin="${1:?bin}"; start="${2:?start}"; stop="${3:?stop}"
    ssh "$HOST" "cd $REF && aarch64-linux-gnu-objdump -d \
      --start-address=$start --stop-address=$stop bin/$bin" ;;
  dec)
    # Принимает ИМЯ функции (надёжнее) либо адрес. ВНИМАНИЕ: Ghidra грузит образ
    # с базой 0x100000 — при передаче runtime-адреса прибавь 0x100000 сам, либо
    # (лучше) передавай имя из `sym`. Логи Ghidra префиксуют только первую строку
    # println, поэтому режем диапазон от маркера "// ====" и снимаем префикс.
    # ⚠️ hermes — НЕ свободная машина: на нём крутятся продовые контейнеры пользователя
    # (specinter-*, litellm, flaresolverr). Ghidra по умолчанию берёт MAXMEM=2G и все ядра —
    # это самая тяжёлая вещь, которую мы туда отправляем. Поэтому: preflight-проверка
    # свободной памяти, жёсткий -Xmx1g, одно ядро и nice 19.
    bin="${1:?bin}"; func="${2:?func|addr}"
    preflight
    ssh "$HOST" "cd $REF && export JAVA_HOME=/usr/lib/jvm/java-21-openjdk-amd64 && \
      export _JAVA_OPTIONS='-Xmx1g' && \
      G=\$(ls -d ghidra_*PUBLIC) && \
      nice -n 19 ./\$G/support/analyzeHeadless $REF proj -process $bin -noanalysis \
        -max-cpu 1 -scriptPath $REF/myscripts -postScript Decompile.java '$func' 2>/dev/null" \
      | sed -n 's/^INFO  Decompile.java> //; /\/\/ ====/,$p' ;;
  run)
    bin="${1:?bin}"; shift || true
    ssh "$HOST" "cd $REF && QEMU_LD_PREFIX=/usr/aarch64-linux-gnu \
      qemu-aarch64-static bin/$bin $*" ;;
  filt)
    ssh "$HOST" 'c++filt' ;;
  *)
    grep '^#' "$0" | sed 's/^# \?//' ; exit 1 ;;
esac
