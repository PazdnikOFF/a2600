#!/usr/bin/env python3
"""revstr.py <mangled_symbol> [--all] — string literals / data syms referenced by a function."""
import re,sys,subprocess,struct
BIN='/Users/pazdnikoff/Documents/PlatformIO/Projects/a2600/update/root/X2000'
d=open(BIN,'rb').read()
e_shoff,=struct.unpack_from('<Q',d,0x28)
e_shentsize,e_shnum,e_shstrndx=struct.unpack_from('<HHH',d,0x3a)
def sh(i):
    o=e_shoff+i*e_shentsize
    return struct.unpack_from('<IIQQQQ',d,o)
sto=sh(e_shstrndx)[4]
SECS=[]
for i in range(e_shnum):
    n,typ,fl,addr,off,size=sh(i)
    nm=d[sto+n:d.index(b'\0',sto+n)].decode()
    if addr: SECS.append((nm,addr,off,size,typ))
def rd(va,n=400):
    for nm,a,o,s,typ in SECS:
        if a<=va<a+s and typ!=8: return d[o+(va-a):o+(va-a)+n]
sym=sys.argv[1]
t=subprocess.run(['objdump','-t',BIN],capture_output=True,text=True).stdout
SYMS={}
addr=size=None
for l in t.splitlines():
    p=l.split()
    if len(p)>=3 and re.match(r'^[0-9a-f]{16}$',p[0]):
        SYMS.setdefault(int(p[0],16),p[-1])
        if p[-1]==sym: addr=int(p[0],16); size=int(p[-2],16)
if addr is None: sys.exit('symbol not found: '+sym)
dis=subprocess.run(['objdump','-d','--start-address=%d'%addr,'--stop-address=%d'%(addr+size),BIN],capture_output=True,text=True).stdout
adrp={}; targets=[]
for l in dis.splitlines():
    m=re.search(r'adrp\s+(x\d+),\s*0x([0-9a-f]+)',l)
    if m: adrp[m.group(1)]=int(m.group(2),16); continue
    m=re.search(r'add\s+(x\d+),\s*(x\d+),\s*#0x([0-9a-f]+)',l)
    if m and m.group(2) in adrp: targets.append(adrp[m.group(2)]+int(m.group(3),16)); continue
    m=re.search(r'ldr\s+x\d+,\s*\[(x\d+),\s*#0x([0-9a-f]+)\]',l)
    if m and m.group(1) in adrp: targets.append(adrp[m.group(1)]+int(m.group(2),16))
seen=set()
for va in targets:
    b=rd(va)
    if b is None:
        n=SYMS.get(va)
        if n and n not in seen: seen.add(n); print('0x%x  BSS %s'%(va,n)); continue
        continue
    s=b.split(b'\x00')[0]
    if len(s)>=1 and all(32<=c<127 for c in s):
        txt=s.decode()
        if txt not in seen: seen.add(txt); print('0x%x  %r'%(va,txt))
    else:
        # try utf16
        n=SYMS.get(va)
        if n:
            if n not in seen: seen.add(n); print('0x%x  SYM %s'%(va,n))
        else:
            u=b[:64]
            key='raw%x'%va
            if key not in seen: seen.add(key); print('0x%x  RAW %s'%(va,u[:32].hex()))
