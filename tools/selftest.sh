#!/usr/bin/env bash
# Регрессия всех self-test-режимов ui_preview (см. docs/PROGRESS.md §4).
# Использование: tools/selftest.sh <путь-к-ui_preview> [repo-root]
# Режимы, пишущие файлы (dccu/account/dcmconf), получают временный ENDO_ROOT.

set -u

BIN="${1:?usage: selftest.sh <ui_preview> [repo-root]}"
ROOT="${2:-$(cd "$(dirname "$0")/.." && pwd)}"
ER="$ROOT/update/root"
export QT_QPA_PLATFORM=offscreen

# Режимы, которым нужен ENDO_ROOT прошивки.
ER_MODES="alg plreg scopecut filt dicom report thesaurus userset coldlight version
statistic sysstatus project style videoset dsdemo videocal templetcfg osdset endoinfo
remoteswitch dcmfmt fxpt language unicodetext encstyle dsreal meaxml templatecfg templatelib encset imageblock reporttmplmgr legalnotice flexbtn osdspin02 imglistcell factoryopt docsync initdocreal"
# Режимы без ENDO_ROOT (чистая логика / своя БД).
PLAIN_MODES="cornercut quickinput examcfg exam filebackup update reportdb
camera endoscope dimming smalllang videoplayer reportedit imgproc exportrec savefile dbservice dispparam pattime recfiles kconfig strutil templateparam dbfileop patstr stopwatch patient doctor dbstr session textblock tableblock titletableblock reporttmpl examdata kobject xmlparser dbsqlite des templetmodel docgen monitor rtcreator videoipc screenmng rtdatasource listbuff quickinputdb quickinputmodel reporttitledb viewbase videolabel rtsimple rtcreators rtte rttext rtimage rttable findcell selectitem docedit movefb changecolor initdoc"
# Режимы, пишущие в ENDO_ROOT → временный каталог.
TMP_MODES="dccu account dcmconf listsetup examno manupwd controlini kcontrolproc templetsave dimmer printdata autotest exambiz lcdproxy"

pass=0; fail=0; failed=""

run() {   # run <mode> <endo_root|->
    local mode="$1" root="$2" out rc
    if [ "$root" = "-" ]; then
        out=$("$BIN" "$mode" 2>&1); rc=$?
    else
        out=$(ENDO_ROOT="$root" "$BIN" "$mode" 2>&1); rc=$?
    fi
    if [ $rc -eq 0 ]; then
        pass=$((pass+1)); printf '  ok   %s\n' "$mode"
    else
        fail=$((fail+1)); failed="$failed $mode"
        printf '  FAIL %s (rc=%d)\n' "$mode" "$rc"
        printf '%s\n' "$out" | tail -3 | sed 's/^/       /'
    fi
}

echo "self-test: $BIN"
for m in $ER_MODES;    do run "$m" "$ER"; done
for m in $PLAIN_MODES; do run "$m" "-";   done
for m in $TMP_MODES;   do
    t=$(mktemp -d); run "$m" "$t"; rm -rf "$t"
done

# db — self-test БД, требует путь к файлу.
t=$(mktemp -d)
if "$BIN" db "$t/out.db" >/dev/null 2>&1; then
    pass=$((pass+1)); echo "  ok   db"
else
    fail=$((fail+1)); failed="$failed db"; echo "  FAIL db"
fi
rm -rf "$t"

echo "---"
echo "PASS: $pass  FAIL: $fail${failed:+ →$failed}"
[ $fail -eq 0 ]
