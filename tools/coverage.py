#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
coverage.py — карта покрытия реверса X-2600.

Запуск из корня репозитория:
    python3 tools/coverage.py > docs/COVERAGE.md

Что делает:
  1. `nm update/root/X2000` -> сам парсит Itanium-мангл (длино-префиксы), т.к. c++filt
     на этом бинарнике не работает. Классы + множества имён методов.
  2. Фильтрует пользовательские классы (K*, AlgParaManager, HmiMcu, ...), отбрасывая
     Qt/std/boost/dcmtk/gst/pugixml/ime_pinyin и сгенерированные Ui_*.
  3. Парсит app/src/**/*.h -> наши классы и объявленные методы (имена 1:1 с оригиналом).
  4. Считает покрытие класс/методы, классифицирует по доменам и off-device-верифицируемости.
  5. `objdump -d` + строки из .rodata/.data*: связывает класс с именами конфигов
     (.ini/.xml/.json/.txt/.qss/.db), на которые ссылаются его методы (adrp+add).

Без внешних зависимостей. Нужны системные `nm` и `objdump` (LLVM/binutils).
"""

import os
import re
import subprocess
import sys
from collections import defaultdict

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN = os.path.join(REPO, "update", "root", "X2000")
SRC = os.path.join(REPO, "app", "src")

# ---------------------------------------------------------------- mangling ---

def comps(s):
    """Разбор последовательности length-prefixed компонент Itanium-мангла."""
    out = []
    i = 0
    while i < len(s) and s[i].isdigit():
        j = i
        while j < len(s) and s[j].isdigit():
            j += 1
        n = int(s[i:j])
        if j + n > len(s):
            break
        out.append(s[j:j + n])
        i = j + n
    return out


def sym_class_method(sym):
    """_ZN<cls><method>E... -> (класс, метод) либо None."""
    if not sym.startswith("_ZN"):
        return None
    c = comps(sym[3:])
    if len(c) < 2:
        return None
    return c[0], "::".join(c[1:])


# ------------------------------------------------------- фильтр «наш класс» ---

THIRD_PARTY = {
    "ime_pinyin", "pugi", "boost", "std", "__gnu_cxx", "QtPrivate", "_GLOBAL__N_1",
    "base", "InputMethod", "yxyDES2",
}
QT_PREFIXES = ("Q",)
DCMTK_PREFIXES = ("Dcm", "OF", "DUL", "DIMSE", "T_ASC", "DVPS", "DSR", "DRT")
GST_PREFIXES = ("Gst", "gst", "G_")

# non-K классы, которые всё-таки прикладные
EXTRA_USER = {
    "HmiMcu", "AlgParaManager", "measure", "report_template", "ColdLight",
    "ReportConfigDlg", "OperateWaitThread", "XmlNode", "XmlParser",
    "languageConfig", "PatientExamData", "exam_detail", "patient", "report_edit",
}


def is_user_class(name):
    if name in EXTRA_USER:
        return True
    if name in THIRD_PARTY:
        return False
    if name.startswith("Ui_"):       # сгенерировано uic
        return False
    if name.startswith("K") and len(name) > 1 and name[1].isupper():
        # KDcm*/KDICOM* — наши обёртки, оставляем
        return True
    if name.startswith(QT_PREFIXES) and len(name) > 1 and name[1].isupper():
        return False
    if name.startswith(DCMTK_PREFIXES) or name.startswith(GST_PREFIXES):
        return False
    return False


# ----------------------------------------------------------------- домены ---
# Порядок важен: первое совпадение выигрывает. Согласовано с docs/ROADMAP.md §2.

DOMAIN_RULES = [
    ("UPDATE", (r"Update", r"^KVersion", r"^KFactoryOptions", r"^KProductsSerial",
                r"^KPublishManager", r"^KEnvConfig")),
    ("DICOM",  (r"Dicom", r"DICOM", r"^KDcm", r"Scu$", r"Scp$", r"^KMPPS", r"Mpps",
                r"Worklist", r"WorkList", r"^KCommitScu", r"Association", r"Requestor",
                r"^KEcho", r"NEventReport", r"^KImage2Dcm", r"^KPdf2Dcm",
                r"^KCFind", r"^KStoreScu")),
    ("REPORT", (r"Report", r"^KRpt", r"^KRT", r"Templet", r"Template", r"Thesaurus",
                r"Thsaurus", r"^KPdf2Pics", r"^KDocumentGenerator", r"^KTextBlock",
                r"^KImageBlock", r"^KTableBlock", r"^KTitleTableBlock",
                r"^KImageEditor", r"^KNewTempletEditor")),
    ("HW",     (r"^KLcd", r"^KCamera", r"^KUsb", r"^KUdisk", r"Printer", r"^KCups",
                r"^KHal", r"^KMemDevice", r"^KStorageDevice", r"Dimming", r"^KScreen",
                r"Temperature", r"Gpio", r"^KPoweroff", r"^KEndoScope", r"^KProcessorSN",
                r"^KDeviceInfo", r"^KComData", r"^KSysPrint", r"^KNetPrint",
                r"^KWindowsPrinter", r"^KProcessorControl", r"^KScopeStaus",
                r"^KScopeValue", r"^KSnMenu", r"^KKeyKits")),
    ("DB",     (r"^KEntity", r"Entity$", r"DBTableHandler", r"DbTableHandler",
                r"^KDb", r"Sqlite", r"^KFileBackup", r"^KExportRecord", r"^KImportRules",
                r"^KSaveFile", r"^KDataFileOpr", r"^KDataOpr", r"^KExamData",
                r"^KRecord", r"^KPatientEntry", r"^KExamEntry", r"^KDecEntry",
                r"^KSessionInfo", r"^KQuickInputData", r"^KExamNoGenerate",
                r"^KMainPatientDataSave", r"^KHistiory", r"^KExamListRecordFileUpdate")),
    ("UI",     (r"Ui$", r"Dlg$", r"Dialog", r"Widget", r"View$", r"Menu", r"Btn",
                r"Button", r"Edit$", r"Label", r"Bar$", r"^KOsd", r"Pinyin", r"Calendar",
                r"Delegate", r"Frame", r"Item$", r"Slider", r"CheckBox", r"Spin",
                r"^KTable", r"^KDialog", r"^KLogin", r"Progress", r"^KMessage",
                r"^KImgList", r"^KImgTable", r"^KImgPushButton", r"^KGridWidget",
                r"^KUIDesktop", r"^KViewBase", r"^KViewSoftEndo", r"^KViewHardEndo",
                r"^KPatientView", r"^KVideoLabel", r"^KVideoPlayer", r"^KPIPView",
                r"^KLayOut", r"^KQRCode", r"^KLine", r"^KFrame", r"^KBackGround",
                r"^KTextEdit", r"^KCustomEdit", r"^KFloatingMsg", r"^KMsgPopup",
                r"^KReservedWidget", r"^KFeature", r"^KExitItem", r"^KIris",
                r"^KUnusedImg", r"^KExamImg", r"^KExamDetail", r"^KMySlider",
                r"Setting$", r"^KLogView", r"^KUiMsgProxy", r"EventFilter",
                r"^KAlgParamAjustDlg", r"^KColdlightAdjust", r"^KVideoCal",
                r"^KScopeInfoEdit", r"^KCameraInfo", r"^KErrorRate", r"^KControlInfo",
                r"^KStatisticInfo", r"^KVersion$", r"^KSysDicom", r"^KUserSrvSet",
                r"^KServerInfo", r"^KAuthMachine", r"^KThirdPartyLegalNotices",
                r"^KOptionListButton", r"^KParamSetBtn", r"^KEmpDateEdit",
                r"^KPasswordEdit", r"^KIpAddrEdit", r"^KIpLineEdit", r"^KPageLineEdit",
                r"^KGooglePinyin", r"^KSmallLang", r"^KQuickInput", r"^KPatientList",
                r"^KExamList.*(Ui|Dlg|Search|View)", r"^KDate", r"^KSpinAge")),
    ("CORE",   (r"^KVideo", r"^KPl", r"^KMainCtrl", r"^KControl", r"^KSystem",
                r"^KAlg", r"^KThread", r"^KAppThread", r"^KProc", r"^KTask",
                r"^KImageProcess", r"^KImgProc", r"^KDccu", r"^KEnc", r"^KStyle",
                r"^KUserOsdSet", r"^KUserSet", r"^KAccount", r"^KProjectSet",
                r"^KColdLightConfig", r"^KDisplayOption", r"^KTimeMng", r"^KTime",
                r"^KAutoTestThread", r"^KSelfTest", r"^KFunTest", r"^KSrv",
                r"^KRealTime", r"^KNormalTime", r"^KObject", r"^KMutex",
                r"^KSemaphore", r"^KConfig", r"^KManuPwdMng", r"^KForceLogout",
                r"^KStatisticConfig", r"^KExamBussinessHandler", r"^KPicMng",
                r"^KVlsMode", r"^KFlexEndo", r"^KRigidEndo", r"^KSaveVideoFile",
                r"^KSaveImageFile", r"^KSaveScreenshotFile", r"^KCommManager",
                r"^KAppThread", r"^KStopWatch", r"^KCostTime")),
]


def domain_of(name):
    for dom, pats in DOMAIN_RULES:
        for p in pats:
            if re.search(p, name):
                return dom
    return "MISC"


# ------------------------------------------- off-device верифицируемость ---

# ❌ device-bound внутри «логических» доменов
NET_DICOM = re.compile(r"Scu$|Scp$|Association|Requestor|^KEcho|^KDicomServer|"
                       r"^KDcmtkLog|NEventReport|^KDcmTaskThread")
UI_REPORT = re.compile(r"Ui$|Dlg$|View|Widget|Editor|Tree|Model|Delegate|Preview")
PURE_MISC = re.compile(r"Config$|Conf$|Set$|Util|String|Time|Data$|Info$|Rules$|"
                       r"Generate|Format|Map$|Cfg$")


def verifiability(name, dom):
    """(значок, причина) — можно ли проверить off-device."""
    if dom == "HW":
        return "❌", "прибор: LCD/сенсор/камера/USB/принтер/MCU"
    if dom == "UI":
        return "❌", "Qt5::Widgets — нужен UX-реверс живого экрана"
    if dom == "DICOM":
        if NET_DICOM.search(name):
            return "❌", "DCMTK-сеть (ассоциации/SCU-SCP)"
        return "⚠️", "БД/конфиг — да, сеть — нет"
    if dom == "REPORT":
        if UI_REPORT.search(name):
            return "❌", "редактор отчётов — Qt-виджеты"
        return "✅", ""
    if dom in ("CORE", "DB", "UPDATE"):
        if re.search(r"Proxy$|Thread$|^KVideoProxy", name):
            return "⚠️", "оркестратор поверх HW-слоя"
        return "✅", ""
    # MISC
    if PURE_MISC.search(name):
        return "✅", ""
    return "⚠️", "смешанное"


# ------------------------------------------------------------ nm / бинарник ---

def read_binary_classes():
    out = subprocess.run(["nm", BIN], capture_output=True, text=True).stdout
    cls = defaultdict(set)
    for line in out.splitlines():
        p = line.split()
        if len(p) < 2:
            continue
        typ = p[1] if len(p) == 2 else p[1]
        if typ not in "TtWw":
            continue
        cm = sym_class_method(p[-1])
        if not cm:
            continue
        c, m = cm
        if not is_user_class(c):
            continue
        # отбрасываем конструкторы/деструкторы/операторы-мангл-хвосты
        cls[c].add(m)
    return cls


# ----------------------------------------------------- наши заголовки ---

DECL_RE = re.compile(r"\b([A-Za-z_]\w*)\s*\(")
SKIP_KW = {
    "if", "for", "while", "switch", "return", "sizeof", "class", "struct", "public",
    "private", "protected", "signals", "slots", "emit", "Q_OBJECT", "explicit",
    "const", "static", "inline", "virtual", "namespace", "enum", "union", "typedef",
    "using", "template", "operator", "connect", "qobject_cast", "static_cast",
}


def read_impl_classes():
    """Наши классы -> множество имён объявленных методов."""
    impl = defaultdict(set)
    for root, _dirs, files in os.walk(SRC):
        for f in files:
            if not f.endswith((".h", ".hpp")):
                continue
            path = os.path.join(root, f)
            with open(path, "r", encoding="utf-8", errors="replace") as fh:
                lines = fh.readlines()
            cur = None
            depth = 0
            for ln in lines:
                s = ln.strip()
                if cur is None:
                    m = re.match(r"^(?:class|struct)\s+(\w+)\s*(?::|$|\{)", s)
                    if m and not s.rstrip().endswith(";"):
                        cur = m.group(1)
                        depth = ln.count("{") - ln.count("}")
                        continue
                    m = re.match(r"^namespace\s+(\w+)\s*\{", s)
                    if m:
                        cur = m.group(1)
                        depth = ln.count("{") - ln.count("}")
                        continue
                else:
                    depth += ln.count("{") - ln.count("}")
                    code = s.split("//")[0]
                    for mm in DECL_RE.finditer(code):
                        nm = mm.group(1)
                        if nm in SKIP_KW or nm == cur:
                            continue
                        impl[cur].add(nm)
                    if depth <= 0:
                        cur = None
    return impl


# ------------------------------------------- строки конфигов из бинарника ---

CFG_RE = re.compile(r"[\w./-]+\.(?:ini|xml|json|qss|db|txt|conf|csv)$")


def rodata_strings():
    strs = {}
    for sec in (".rodata", ".data.rel.ro", ".data"):
        out = subprocess.run(["objdump", "-s", "-j", sec, BIN],
                             capture_output=True, text=True).stdout
        start = None
        buf = bytearray()
        blocks = []
        for line in out.splitlines():
            m = re.match(r"^\s*([0-9a-f]{6,16})\s((?:[0-9a-f]{2,8}\s){1,4})\s", line)
            if not m:
                continue
            addr = int(m.group(1), 16)
            b = bytes.fromhex(m.group(2).replace(" ", ""))
            if start is None:
                start, buf = addr, bytearray(b)
            elif addr == start + len(buf):
                buf += b
            else:
                blocks.append((start, bytes(buf)))
                start, buf = addr, bytearray(b)
        if start is not None:
            blocks.append((start, bytes(buf)))
        for base, blob in blocks:
            for m in re.finditer(rb"[\x20-\x7e]{4,}", blob):
                s = m.group().decode()
                if CFG_RE.search(s):
                    strs[base + m.start()] = s
    return strs


FUNC_RE = re.compile(r"^[0-9a-f]+ <(\S+)>:")
ADRP_RE = re.compile(r"\tadrp\t(\w+), (0x[0-9a-f]+)")
ADD_RE = re.compile(r"\tadd\t(\w+), (\w+), #(0x[0-9a-f]+|\d+)")


def class_configs(cfg_strings):
    """class -> set(имена конфигов), по adrp+add внутри методов класса."""
    res = defaultdict(set)
    p = subprocess.Popen(["objdump", "-d", BIN], stdout=subprocess.PIPE, text=True)
    cur = None
    regs = {}
    for line in p.stdout:
        m = FUNC_RE.match(line)
        if m:
            cm = sym_class_method(m.group(1).split("@")[0])
            cur = cm[0] if cm and is_user_class(cm[0]) else None
            regs = {}
            continue
        if cur is None:
            continue
        m = ADRP_RE.search(line)
        if m:
            regs[m.group(1)] = int(m.group(2), 16)
            continue
        m = ADD_RE.search(line)
        if m:
            dst, src, imm = m.group(1), m.group(2), m.group(3)
            base = regs.get(src)
            if base is None:
                continue
            off = int(imm, 16) if imm.startswith("0x") else int(imm)
            a = base + off
            if a in cfg_strings:
                res[cur].add(cfg_strings[a])
            regs.pop(dst, None)
    p.stdout.close()
    p.wait()
    return res


# ------------------------------------------------------------------ отчёт ---

def fmt_cfgs(cfgs, limit=3):
    if not cfgs:
        return "—"
    items = sorted(cfgs, key=lambda s: (len(s), s))
    short = [os.path.basename(x) for x in items]
    seen = []
    for s in short:
        if s not in seen:
            seen.append(s)
    out = ", ".join("`%s`" % s for s in seen[:limit])
    if len(seen) > limit:
        out += " +%d" % (len(seen) - limit)
    return out


def main():
    if not os.path.exists(BIN):
        sys.exit("нет бинарника: %s" % BIN)

    binc = read_binary_classes()
    impl = read_impl_classes()
    cfgs = class_configs(rodata_strings())

    rows = []
    for cname, methods in binc.items():
        dom = domain_of(cname)
        ok, why = verifiability(cname, dom)
        mine = impl.get(cname, set())
        done = len(methods & mine)
        rows.append({
            "cls": cname, "dom": dom, "total": len(methods), "done": done,
            "pct": (100.0 * done / len(methods)) if methods else 0.0,
            "ver": ok, "why": why, "cfg": cfgs.get(cname, set()),
            "impl": cname in impl,
        })

    tot_cls = len(rows)
    tot_meth = sum(r["total"] for r in rows)
    done_cls = sum(1 for r in rows if r["done"] > 0)
    done_meth = sum(r["done"] for r in rows)

    W = sys.stdout.write
    W("# Карта покрытия реверса X-2600\n\n")
    W("> Сгенерировано: `python3 tools/coverage.py > docs/COVERAGE.md`\n")
    W("> Источники: `nm update/root/X2000` (свой парсер Itanium-мангла — `c++filt`\n")
    W("> на этом бинарнике не работает), `objdump -d` + строки `.rodata`/`.data*`\n")
    W("> для привязки конфигов, `app/src/**/*.h` — наша сторона (сверка по именам; см. §5.1).\n")
    W("> Домены и off-device-признак — эвристики имён, согласованы с `docs/ROADMAP.md §2`.\n\n")

    W("## 1. Сводка\n\n")
    W("| Метрика | Значение |\n|---|---|\n")
    W("| Пользовательских классов в референсе | **%d** |\n" % tot_cls)
    W("| Методов в них (уникальные имена) | **%d** |\n" % tot_meth)
    W("| Классов затронуто у нас | **%d** (%.0f%%) |\n" % (done_cls, 100.0 * done_cls / tot_cls))
    W("| Методов реализовано (совпало по имени) | **%d** (%.1f%%) |\n" % (done_meth, 100.0 * done_meth / tot_meth))
    W("\n")

    W("## 2. По доменам\n\n")
    W("| Домен | Классов | Методов | Реализовано методов | % | Классов затронуто |\n")
    W("|---|---:|---:|---:|---:|---:|\n")
    agg = defaultdict(lambda: [0, 0, 0, 0])
    for r in rows:
        a = agg[r["dom"]]
        a[0] += 1
        a[1] += r["total"]
        a[2] += r["done"]
        a[3] += 1 if r["done"] > 0 else 0
    for dom in sorted(agg, key=lambda d: -agg[d][1]):
        c, t, d, dc = agg[dom]
        W("| **%s** | %d | %d | %d | %.1f%% | %d |\n" % (dom, c, t, d, 100.0 * d / t if t else 0, dc))
    W("\n")

    W("## 3. Верифицируемость off-device\n\n")
    W("| Признак | Классов | Методов |\n|---|---:|---:|\n")
    vagg = defaultdict(lambda: [0, 0])
    for r in rows:
        vagg[r["ver"]][0] += 1
        vagg[r["ver"]][1] += r["total"]
    for v in ("✅", "⚠️", "❌"):
        if v in vagg:
            W("| %s | %d | %d |\n" % (v, vagg[v][0], vagg[v][1]))
    W("\n")

    W("## 4. ТОП-40 нереализованных off-device-классов (✅)\n\n")
    W("Кандидаты на следующие итерации: чистая логика/конфиги, тестируются self-test'ом на Mac.\n\n")
    W("| Класс | Домен | Методов | Конфиги | Заметка |\n|---|---|---:|---|---|\n")
    cand = [r for r in rows if r["ver"] == "✅" and r["done"] == 0]
    cand.sort(key=lambda r: -r["total"])
    for r in cand[:40]:
        note = "есть заголовок, но свой API — сверить имена" if r["impl"] else "не начат"
        W("| `%s` | %s | %d | %s | %s |\n" % (r["cls"], r["dom"], r["total"], fmt_cfgs(r["cfg"]), note))
    W("\n")

    W("## 5. Частично реализованные (есть методы, но не все)\n\n")
    W("| Класс | Домен | Методов | Наших | Покрытие |\n|---|---|---:|---:|---:|\n")
    part = [r for r in rows if 0 < r["done"] < r["total"]]
    part.sort(key=lambda r: -(r["total"] - r["done"]))
    for r in part[:25]:
        W("| `%s` | %s | %d | %d | %.0f%% |\n" % (r["cls"], r["dom"], r["total"], r["done"], r["pct"]))
    W("\n")

    W("### 5.1 Оговорка о точности метрики\n\n")
    reshaped = sorted(r["cls"] for r in rows if r["impl"] and r["done"] == 0)
    own = sorted(set(impl) - set(binc))
    W("Счётчик «реализовано» = **совпадение имени метода** с референсом, т.е. это **нижняя\n")
    W("оценка**. Там, где мы сознательно переосмыслили API (свои имена вместо оригинальных),\n")
    W("метрика показывает 0 при фактически рабочем коде:\n\n")
    W("- **Свой API при наличии класса-референса (%d):** %s\n\n"
      % (len(reshaped), ", ".join("`%s`" % c for c in reshaped) if reshaped else "—"))
    W("- **Наши абстракции без класса-референса (%d):** %s\n\n"
      % (len(own), ", ".join("`%s`" % c for c in own) if own else "—"))
    W("Такие классы стоит либо привести к именам оригинала, либо исключить из метрики руками.\n\n")

    W("## 6. Device-bound (❌) — крупнейшие\n\n")
    W("Требуют прибора/живого экрана; off-device проверяются только косвенно.\n\n")
    W("| Класс | Домен | Методов | Причина |\n|---|---|---:|---|\n")
    db = [r for r in rows if r["ver"] == "❌"]
    db.sort(key=lambda r: -r["total"])
    for r in db[:30]:
        W("| `%s` | %s | %d | %s |\n" % (r["cls"], r["dom"], r["total"], r["why"]))
    W("\n")

    W("## 7. Как перегенерировать\n\n")
    W("```sh\npython3 tools/coverage.py > docs/COVERAGE.md\n```\n\n")
    W("Правила доменов/верифицируемости — `DOMAIN_RULES` и `verifiability()` в `tools/coverage.py`.\n")


if __name__ == "__main__":
    main()
