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
    bin="${1:?bin}"; func="${2:?func|addr}"
    ssh "$HOST" "cd $REF && export JAVA_HOME=/usr/lib/jvm/java-21-openjdk-amd64 && \
      G=\$(ls -d ghidra_*PUBLIC) && \
      ./\$G/support/analyzeHeadless $REF proj -process $bin -noanalysis \
        -scriptPath $REF -postScript Decompile.java '$func' 2>/dev/null \
        | sed -n '/^\/\/ ====/,\$p'" ;;
  run)
    bin="${1:?bin}"; shift || true
    ssh "$HOST" "cd $REF && QEMU_LD_PREFIX=/usr/aarch64-linux-gnu \
      qemu-aarch64-static bin/$bin $*" ;;
  filt)
    ssh "$HOST" 'c++filt' ;;
  *)
    grep '^#' "$0" | sed 's/^# \?//' ; exit 1 ;;
esac
