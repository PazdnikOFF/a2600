#include "sys/KSmallLangTranslate.h"

#include "kernel/KSystemLog.h"
#include "sys/KSmallLangTables.h"

using namespace smalllang;

namespace {

const char *const kSrcFile = "platform/language/SmallLanguage/KSmallLangTranslate.cpp";
const char *const kInfo    = "[info]";

bool g_capsLockOn = false;

// Реф. .rodata @0x89bce0 — 5 x {клавиша, маска}. AltGr (0x468) и Fn (0x464)
// НАМЕРЕННО не экспортируются в nModState.
struct ModMask { E_SONO_KEY eKey; int mask; };
const ModMask kModStateMap[5] = {
    { 0x456, 0x0001 }, { 0x462, 0x0002 }, { 0x463, 0x0040 },
    { 0x466, 0x0100 }, { 0x449, 0x2000 },
};

// Реф. .rodata @0x89bd68 — 5 x {out, in}: подстановка после мёртвой клавиши.
struct Remap { E_SONO_KEY out; E_SONO_KEY in; };
const Remap kDeadKeyRemap[5] = {
    { 0x42d, 0x54f }, { 0x454, 0x550 }, { 0x479, 0x551 },
    { 0x47a, 0x552 }, { 0x4b2, 0x553 },
};

// Реф. имена клавиш выдаёт KKey2Name::GetNameOfKey, читающий массивы
// g_strKeysym_S40/S50 из .bss (их расшифровка — отдельная задача).
// Для логов достаточно кода.
std::string GetNameOfKey(E_SONO_KEY k) { return std::to_string(k); }

// Раскладки. aeBaseModifier/aeLockedModifier/aeCombPrefixKey и указатели —
// из реверса структур g_st<Язык>KeyboardLayout (по 128 байт).
stKbdLayout g_stLatinKeyboardLayout = {
    0,
    { 0x456, 0x462, 0x463, 0x464, 0x466, 0x468, 0, 0 },
    { 0x449, 0, 0, 0, 0, 0, 0, 0 },
    { 0x42d, 0x454, 0x479, 0x47a, 0x4b2, 0, 0, 0 },
    0, g_astLatinDetail_CmbChar, g_astModBitConf_S50, g_astLatinKbdDetail,
};
stKbdLayout g_stRussianKeyboardLayout_Endo = {
    0,
    { 0x456, 0x462, 0x466, 0x468, 0, 0, 0, 0 },   // без Ctrl/Fn
    { 0x449, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },                   // мёртвых клавиш нет
    0, nullptr, g_astModBitConf_S50, g_astRussianKbdDetail,
};
stKbdLayout g_stFrenchKeyboardLayout = {
    0,
    { 0x456, 0x462, 0x463, 0x464, 0x466, 0x468, 0, 0 },
    { 0x449, 0, 0, 0, 0, 0, 0, 0 },
    { 0x554, 0x479, 0, 0, 0, 0, 0, 0 },
    0, g_astFrenchDetail_CmbChar, g_astModBitConf_S50, g_astFrenchKbdDetail,
};
stKbdLayout g_stPolishKeyboardLayout = {
    0,
    { 0x456, 0x462, 0x463, 0x464, 0x466, 0x468, 0, 0 },
    { 0x449, 0, 0, 0, 0, 0, 0, 0 },
    { 0x558, 0x55d, 0, 0, 0, 0, 0, 0 },
    0, g_astPolishDetail_CmbChar, g_astModBitConf_S50, g_astPolishKbdDetail,
};
stKbdLayout g_stHungaryKeyboardLayout = {
    0,
    { 0x456, 0x462, 0x463, 0x464, 0x466, 0x468, 0, 0 },
    { 0x449, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    0, nullptr, g_astModBitConf_S50, g_astHungaryKbdDetail,
};

} // namespace

void KSmallLangTranslate::SetCapsLockOn(bool on) { g_capsLockOn = on; }
bool KSmallLangTranslate::IsCapsLockOn() { return g_capsLockOn; }

KSmallLangTranslate &KSmallLangTranslate::GetInstance()
{
    static KSmallLangTranslate inst;   // реф.: guarded static + __cxa_atexit
    return inst;
}

// --- битовые примитивы ------------------------------------------------------

void KSmallLangTranslate::SetBit(char *p, int nIdx, int bSet)
{
    const char m = char(1 << (nIdx % 8));
    if (bSet)
        p[nIdx / 8] |= m;
    else
        p[nIdx / 8] &= ~m;
}

int KSmallLangTranslate::GetBit(char *p, int nIdx)
{
    // ⚠️ КВИРК: возвращается СЫРАЯ МАСКА, а не 0/1 (и без sxtb, в отличие от SetBit).
    return (unsigned char)(p[nIdx / 8]) & (1 << (nIdx % 8));
}

// --- инициализация ----------------------------------------------------------

bool KSmallLangTranslate::KbdLayoutInit(const std::string &strLang)
{
    // Реф. порядок сравнений: Latin, Russian, French, Polish, Hungary.
    if (strLang.compare("Latin") == 0)
        m_pKbdLayout = &g_stLatinKeyboardLayout;
    else if (strLang.compare("Russian") == 0)
        m_pKbdLayout = &g_stRussianKeyboardLayout_Endo;
    else if (strLang.compare("French") == 0)
        m_pKbdLayout = &g_stFrenchKeyboardLayout;
    else if (strLang.compare("Polish") == 0)
        m_pKbdLayout = &g_stPolishKeyboardLayout;
    else if (strLang.compare("Hungary") == 0)
        m_pKbdLayout = &g_stHungaryKeyboardLayout;
    else {
        LogPrintfEx(true, "[APP][E]: ", "Unknown Language Type!!!\n");
        return true;   // ⚠️ КВИРК: true и при неизвестном языке, раскладка не тронута
    }
    m_pKbdLayout->nModBitState = 0;
    return true;
}

// --- состояние модификаторов ------------------------------------------------

int KSmallLangTranslate::GetCurrentModState(stKbdLayout *pL)
{
    int local = pL->nModBitState;
    int result = 0;
    for (const stModBitConf *c = pL->pModBitConf; c->eKey != 0; ++c) {
        if (!GetBit(reinterpret_cast<char *>(&local), c->nBitIdx))
            continue;
        // Реф.: линейный поиск по 5 записям, БЕЗ раннего выхода из внешнего цикла.
        for (int j = 0; j < 5; ++j)
            if (kModStateMap[j].eKey == c->eKey)
                result |= kModStateMap[j].mask;
    }
    return result;
}

bool KSmallLangTranslate::IsBaseModifier(stKbdLayout *pL, E_SONO_KEY key)
{
    for (int i = 0; i < 8 && pL->aeBaseModifier[i] != 0; ++i)
        if (pL->aeBaseModifier[i] == key)
            return true;
    return false;
}

bool KSmallLangTranslate::IsLockedModifier(stKbdLayout *pL, E_SONO_KEY key)
{
    for (int i = 0; i < 8 && pL->aeLockedModifier[i] != 0; ++i)
        if (pL->aeLockedModifier[i] == key)
            return true;
    return false;
}

int KSmallLangTranslate::GetModifierBitIndex(stKbdLayout *pL, E_SONO_KEY key)
{
    for (const stModBitConf *c = pL->pModBitConf; c->eKey != 0; ++c)
        if (c->eKey == key)
            return c->nBitIdx;
    return -1;
}

int KSmallLangTranslate::KbdLayout_GetModifierStatus(int /*nUnused*/, E_SONO_KEY key)
{
    stKbdLayout *pL = m_pKbdLayout;   // реф.: первый параметр затирается здесь же
    if (!pL) {
        LogPrintfx(kInfo, kSrcFile, 127, "KbdLayout_GetModifierStatus", "err layout data");
        return -2;
    }
    const int idx = GetModifierBitIndex(pL, key);
    if (idx == -1)
        return -1;
    return GetBit(reinterpret_cast<char *>(&pL->nModBitState), idx);
}

int KSmallLangTranslate::GetAltgrModifierStatus()
{
    stKbdLayout *pL = m_pKbdLayout;
    if (!pL)
        return -1;
    if (!IsBaseModifier(pL, KEY_ALTGR))
        return -1;
    const int idx = GetModifierBitIndex(pL, KEY_ALTGR);
    if (idx == -1)
        return -1;
    return GetBit(reinterpret_cast<char *>(&pL->nModBitState), idx);
}

int KSmallLangTranslate::GetRussianAltgrLockStatus()
{
    stKbdLayout *pL = m_pKbdLayout;
    if (!pL)
        return -1;
    if (!IsLockedModifier(pL, KEY_ALTGR))
        return -1;
    const int idx = GetModifierBitIndex(pL, KEY_ALTGR);
    if (idx == -1)
        return -1;
    return GetBit(reinterpret_cast<char *>(&pL->nModBitState), idx);
}

int KSmallLangTranslate::ProcBaseModifier(stKbdLayout *pL, E_SONO_KEY key, int bPressed)
{
    const int idx = GetModifierBitIndex(pL, key);
    if (idx == -1)
        return -1;
    SetBit(reinterpret_cast<char *>(&pL->nModBitState), idx, bPressed != 0);
    return 0;
}

int KSmallLangTranslate::ProcLockedModifier(stKbdLayout *pL, E_SONO_KEY key, int bPressed)
{
    // ⚠️ КВИРК: отпускание игнорируется, возвращается сам bPressed (0).
    if (bPressed == 0)
        return bPressed;
    const int idx = GetModifierBitIndex(pL, key);
    if (idx == -1)
        return -1;
    char *base = reinterpret_cast<char *>(&pL->nModBitState);
    SetBit(base, idx, GetBit(base, idx) == 0);   // переключение
    return 0;
}

// --- многоуровневые клавиши -------------------------------------------------

bool KSmallLangTranslate::IsMultiLevelKey(stKbdLayout *pL, E_SONO_KEY key)
{
    if (!pL->pKbdDetail)
        return false;
    for (const stKbdDetail *d = pL->pKbdDetail; d->eKey != 0; ++d)
        if (d->eKey == key)
            return true;
    return false;
}

int KSmallLangTranslate::ProcMultiLevelKey(stKbdLayout *pL, E_SONO_KEY *peKey,
                                           int /*nUnused*/)
{
    // Сборка 4-битного индекса из активных модификаторов.
    int modIdx = 0;
    for (const stModBitConf *c = pL->pModBitConf; c->eKey != 0; ++c) {
        if (GetBit(reinterpret_cast<char *>(&pL->nModBitState), c->nBitIdx)
            && c->nOutIdxBit != -1)
            SetBit(reinterpret_cast<char *>(&modIdx), c->nOutIdxBit, 1);
    }

    const stKbdDetail *entry = nullptr;
    if (pL->pKbdDetail)
        for (const stKbdDetail *d = pL->pKbdDetail; d->eKey != 0; ++d)
            if (d->eKey == *peKey) {
                entry = d;
                break;
            }
    if (!entry)
        return -1;

    const int lvl = entry->piModCmb2OutKeyArrIdx[modIdx];
    if (lvl == -1) {
        *peKey = 0;      // клавиша проглатывается
        return 0;
    }
    // ⚠️ Обе ветки ниже НЕДОСТИЖИМЫ с поставляемыми таблицами (там только 0..3
    // и -1) — мёртвые сравнения оригинала сохранены.
    if (lvl < 0) {
        LogPrintfx(kInfo, kSrcFile, 457, "ProcMultiLevelKey",
                   "err print sym index %c key=%s", lvl, GetNameOfKey(*peKey).c_str());
        return -1;
    }
    if (lvl > 6) {
        LogPrintfx(kInfo, kSrcFile, 464, "ProcMultiLevelKey",
                   "err2 print sym index %c key=%s", lvl, GetNameOfKey(*peKey).c_str());
        return -1;
    }
    const E_SONO_KEY out = entry->aeOutKey[lvl];
    if (out == 0) {
        *peKey = 0;
        return 0;
    }
    // Реф. логирует КАЖДОЕ успешное преобразование (горячий путь).
    LogPrintfx(kInfo, kSrcFile, 482, "ProcMultiLevelKey", "convert %s to %s",
               GetNameOfKey(*peKey).c_str(), GetNameOfKey(out).c_str());
    *peKey = out;
    return 0;
}

int KSmallLangTranslate::SyncCapslockStatus(stKbdLayout *pL, E_SONO_KEY key)
{
    const int idx = GetModifierBitIndex(pL, key);
    if (idx == -1)
        return -1;
    SetBit(reinterpret_cast<char *>(&pL->nModBitState), idx, IsCapsLockOn());
    return 0;
}

int KSmallLangTranslate::ProcOneRawKeysym(int /*nUnused*/, E_SONO_KEY *peKey, int bPressed)
{
    if (!peKey) {
        LogPrintfx(kInfo, kSrcFile, 193, "ProcOneRawKeysym", "err keysym");
        return -1;
    }
    stKbdLayout *pL = m_pKbdLayout;
    if (!pL) {
        LogPrintfx(kInfo, kSrcFile, 200, "ProcOneRawKeysym", "err pLayout data");
        return -2;
    }
    // ⚠️ ЛЮБОЕ событие AltGr (нажатие ИЛИ отпускание) сбрасывает защёлку.
    if (*peKey == KEY_ALTGR)
        pL->ePendingCombPrefix = 0;

    if (SyncCapslockStatus(pL, KEY_CAPSLOCK) == -1)
        // ⚠️ Сбой логируется и ИГНОРИРУЕТСЯ — работа продолжается.
        LogPrintfEx(true, "[APP][E]: ", "Sync Capslock Status Failed!!!\n");

    if (IsBaseModifier(pL, *peKey))
        return ProcBaseModifier(pL, *peKey, bPressed);
    if (IsLockedModifier(pL, *peKey))
        return ProcLockedModifier(pL, *peKey, bPressed);
    if (IsMultiLevelKey(pL, *peKey))
        return ProcMultiLevelKey(pL, peKey, bPressed);
    return 0;
}

// --- мёртвые клавиши --------------------------------------------------------

int KSmallLangTranslate::GetCombPrefixArrayIdx(stKbdLayout *pL, E_SONO_KEY key)
{
    // Реф.: сканируются ВСЕ 8 слотов безусловно (нулевые просто не совпадают).
    for (int i = 0; i < 8; ++i)
        if (pL->aeCombPrefixKey[i] == key)
            return i;
    return -1;
}

bool KSmallLangTranslate::IsCombinePrefixKey(stKbdLayout *pL, E_SONO_KEY key)
{
    return GetCombPrefixArrayIdx(pL, key) != -1;
}

int KSmallLangTranslate::ProcCombStepKey(stKbdLayout *pL, stPADKeyboardEvent *pEvt,
                                         int nMaxCnt)
{
    const stCmbCharDetail *entry = nullptr;
    if (pL->pCmbChar)
        for (const stCmbCharDetail *c = pL->pCmbChar; c->eBaseKey != 0; ++c)
            if (c->eBaseKey == pEvt[0].eKey) {
                entry = c;
                break;
            }

    if (entry) {
        const int idx = GetCombPrefixArrayIdx(pL, pL->ePendingCombPrefix);
        const E_SONO_KEY out = (idx >= 0) ? entry->aeOut[idx] : 0;
        if (out != 0) {
            pL->ePendingCombPrefix = 0;
            pEvt[0].eKey = out;
            pEvt[0].nModState = GetCurrentModState(pL);
            return 1;
        }
    }

    // Совпадения нет (или вывод нулевой) — переигрываем три события.
    if (nMaxCnt <= 2) {
        LogPrintfx(kInfo, kSrcFile, 571, "ProcCombStepKey",
                   "key buffer too small (%d), at least 3.\n", nMaxCnt);
        return 0;
    }
    const E_SONO_KEY pending = pL->ePendingCombPrefix;
    const int mod = GetCurrentModState(pL);
    pEvt[2].bRelease = pEvt[0].bRelease;
    pEvt[2].eKey = pEvt[0].eKey;
    pEvt[0].bRelease = 0;
    pEvt[0].eKey = pending;
    pEvt[0].nModState = mod;
    pEvt[1].bRelease = 1;
    pEvt[1].eKey = pending;
    pEvt[1].nModState = mod;
    pL->ePendingCombPrefix = 0;
    return 3;
}

int KSmallLangTranslate::ProcCombineKey(stKbdLayout *pL, stPADKeyboardEvent *pEvt,
                                        int nMaxCnt)
{
    const E_SONO_KEY k = pEvt[0].eKey;
    const E_SONO_KEY p = pL->ePendingCombPrefix;

    if (pEvt[0].bRelease == 0) {          // нажатие
        if (p == 0) {
            if (IsCombinePrefixKey(pL, k)) {
                pL->ePendingCombPrefix = k;
                return 0;                  // проглатываем префикс
            }
        } else {
            return ProcCombStepKey(pL, pEvt, nMaxCnt);
        }
    } else {                               // отпускание
        if (p == 0) {
            if (IsCombinePrefixKey(pL, k))
                return 0;
        } else if (p == k) {
            return 0;
        } else {
            return ProcCombStepKey(pL, pEvt, nMaxCnt);
        }
    }

    // Путь подстановки.
    pEvt[0].nModState = GetCurrentModState(pL);
    // ⚠️ Реф.: цикл БЕЗ раннего выхода — все 5 записей всегда сравниваются
    // (с текущими данными безвредно, но форма цикла сохранена).
    for (int i = 0; i < 5; ++i)
        if (kDeadKeyRemap[i].in == pEvt[0].eKey)
            pEvt[0].eKey = kDeadKeyRemap[i].out;
    return 1;
}

int KSmallLangTranslate::KbdLayout_ProcRawKeysym(int /*nUnused*/,
                                                 stPADKeyboardEvent *pEvt, int nMaxCnt)
{
    if (!pEvt) {
        LogPrintfx(kInfo, kSrcFile, 80, "KbdLayout_ProcRawKeysym", "err keysym");
        return -1;
    }
    stKbdLayout *pL = m_pKbdLayout;
    if (!pL) {
        LogPrintfx(kInfo, kSrcFile, 87, "KbdLayout_ProcRawKeysym", "err layout data");
        return -2;
    }
    E_SONO_KEY key = pEvt[0].eKey;
    const int r = ProcOneRawKeysym(0, &key, pEvt[0].bRelease == 0);
    if (r != 0) {
        // ⚠️ КВИРК: ошибка ПРОГЛАТЫВАЕТСЯ — наружу уходит 0, а не код ошибки.
        LogPrintfx(kInfo, kSrcFile, 98, "KbdLayout_ProcRawKeysym",
                   "Translate is Error, i_ret[%d]", r);
        return 0;
    }
    pEvt[0].eKey = key;
    if (GetModifierBitIndex(pL, pEvt[0].eKey) != -1) {
        // ⚠️ Клавиша-модификатор отдаётся как ОДНО пригодное событие (1),
        // а не поглощается.
        pEvt[0].nModState = GetCurrentModState(pL);
        return 1;
    }
    return ProcCombineKey(pL, pEvt, nMaxCnt);
}
