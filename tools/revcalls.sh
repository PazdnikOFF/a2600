#!/bin/bash
# revcalls.sh <mangled_symbol> — последовательность вызовов функции из X2000
BIN=/Users/pazdnikoff/Documents/PlatformIO/Projects/a2600/update/root/X2000
SYM="$1"
line=$(objdump -t "$BIN" 2>/dev/null | awk -v s="$SYM" '$NF==s{print $1, $(NF-1)}')
addr=0x$(echo $line | awk '{print $1}')
size=0x$(echo $line | awk '{print $2}')
[ -z "$line" ] && { echo "symbol not found: $SYM"; exit 1; }
start=$((addr)); stop=$((addr+size))
objdump -d --start-address=$start --stop-address=$stop "$BIN" 2>/dev/null \
 | grep -E '\tbl\t' | grep -oE '<[^>]+>' | \
python3 -c '
import re,sys
def comps(s):
    out=[];i=0
    while i<len(s) and s[i].isdigit():
        j=i
        while j<len(s) and s[j].isdigit():j+=1
        n=int(s[i:j]);out.append(s[j:j+n]);i=j+n
    return out
n=0
for l in sys.stdin:
    sym=l.strip().strip("<>")
    if sym.startswith("_Z"):
        c=comps(re.sub(r"^_ZN?K?","",sym) if sym.startswith("_ZN") else re.sub(r"^_Z","",sym))
        name="::".join(c[:2]) if len(c)>=2 else (c[0] if c else sym)
    else:
        name=sym
    n+=1; print(f"{n:3d}. {name}")
'
