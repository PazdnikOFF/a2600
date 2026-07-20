#pragma once

#include <string>

#include "sys/KSmallLangTypes.h"

// Трансляция клавиш «малых языков» (реф. KSmallLangTranslate, X-2600;
// исходник реф. — platform/language/SmallLanguage/KSmallLangTranslate.cpp).
//
// РЕВЕРС: Мейерсов синглтон с ОДНИМ полем, sizeof == 8; ни vtable, ни typeinfo,
// дтор тривиальный. Qt не используется вообще, железа нет — класс полностью
// проверяем off-device (единственная зацепка — KKeyKits::IsCapsLockOn, вынесена
// в подменяемый шов).
//
// Конвейер: сырой E_SONO_KEY → диспетчер (базовый модификатор / залипающий
// модификатор / многоуровневая клавиша) → modIdx (4 бита) →
// piModCmb2OutKeyArrIdx[modIdx] → уровень 0..6 → aeOutKey[уровень].
class KSmallLangTranslate
{
public:
    static KSmallLangTranslate &GetInstance();

    // --- битовые примитивы --------------------------------------------------
    // Реф.: знаковые C-шные `/` и `%` по индексу.
    static void SetBit(char *p, int nIdx, int bSet);
    // ⚠️ КВИРК: возвращает СЫРУЮ МАСКУ (0/1/2/4/…/64), а НЕ 0/1 — и это значение
    // прокидывается наружу через KbdLayout_GetModifierStatus и Get*Status.
    static int  GetBit(char *p, int nIdx);

    // --- инициализация ------------------------------------------------------
    // Языки: "Latin", "Russian", "French", "Polish", "Hungary".
    // ⚠️ КВИРК: при НЕИЗВЕСТНОМ языке логирует ошибку, НО ВСЁ РАВНО возвращает
    // true, оставив прежнюю раскладку (возможно, nullptr).
    bool KbdLayoutInit(const std::string &strLang);

    // --- состояние модификаторов --------------------------------------------
    int  GetCurrentModState(smalllang::stKbdLayout *pL);
    bool IsBaseModifier(smalllang::stKbdLayout *pL, E_SONO_KEY key);
    bool IsLockedModifier(smalllang::stKbdLayout *pL, E_SONO_KEY key);
    int  GetModifierBitIndex(smalllang::stKbdLayout *pL, E_SONO_KEY key);
    // ⚠️ Первый параметр в реф. МЁРТВ (сразу перезаписывается m_pKbdLayout) —
    // сохранён в сигнатуре ради верности. -2 при отсутствии раскладки.
    int  KbdLayout_GetModifierStatus(int nUnused, E_SONO_KEY key);
    int  GetAltgrModifierStatus();
    int  GetRussianAltgrLockStatus();

    int  ProcBaseModifier(smalllang::stKbdLayout *pL, E_SONO_KEY key, int bPressed);
    // ⚠️ КВИРК: при bPressed == 0 сразу возвращает bPressed (0) — отпускание
    // игнорируется; это ПЕРЕКЛЮЧАТЕЛЬ, а не установка.
    int  ProcLockedModifier(smalllang::stKbdLayout *pL, E_SONO_KEY key, int bPressed);

    // --- многоуровневые клавиши ---------------------------------------------
    bool IsMultiLevelKey(smalllang::stKbdLayout *pL, E_SONO_KEY key);
    // ⚠️ Третий параметр в реф. НЕ ЧИТАЕТСЯ.
    int  ProcMultiLevelKey(smalllang::stKbdLayout *pL, E_SONO_KEY *peKey, int nUnused);
    int  SyncCapslockStatus(smalllang::stKbdLayout *pL, E_SONO_KEY key);
    // ⚠️ Первый параметр мёртв. Сбой SyncCapslockStatus ЛОГИРУЕТСЯ И ИГНОРИРУЕТСЯ.
    // ⚠️ ЛЮБОЕ событие AltGr (0x468), нажатие ИЛИ отпускание, сбрасывает защёлку
    //    мёртвой клавиши.
    int  ProcOneRawKeysym(int nUnused, E_SONO_KEY *peKey, int bPressed);

    // --- мёртвые клавиши (комбинации) ---------------------------------------
    int  GetCombPrefixArrayIdx(smalllang::stKbdLayout *pL, E_SONO_KEY key);
    bool IsCombinePrefixKey(smalllang::stKbdLayout *pL, E_SONO_KEY key);
    // Возвращает число заполненных событий: 1 (совпало) или 3 (переигрывание),
    // либо 0, если буфера не хватило (нужно >= 3).
    int  ProcCombStepKey(smalllang::stKbdLayout *pL,
                         smalllang::stPADKeyboardEvent *pEvt, int nMaxCnt);
    int  ProcCombineKey(smalllang::stKbdLayout *pL,
                        smalllang::stPADKeyboardEvent *pEvt, int nMaxCnt);
    // ⚠️ Первый параметр мёртв. При сбое ProcOneRawKeysym логирует и возвращает
    //    0 (ошибка ПРОГЛАТЫВАЕТСЯ, код наружу не идёт).
    int  KbdLayout_ProcRawKeysym(int nUnused,
                                 smalllang::stPADKeyboardEvent *pEvt, int nMaxCnt);

    // Не из реф. — доступ и шов для self-test.
    smalllang::stKbdLayout *Layout() const { return m_pKbdLayout; }
    // Реф. берёт состояние CapsLock у KKeyKits::GetInstance().IsCapsLockOn()
    // (единственная device-зацепка класса).
    static void SetCapsLockOn(bool on);
    static bool IsCapsLockOn();

private:
    KSmallLangTranslate() = default;

    smalllang::stKbdLayout *m_pKbdLayout = nullptr;   // 0x00
};
