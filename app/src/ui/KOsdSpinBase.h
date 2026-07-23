#pragma once

#include "KFrame.h"

// Навигационная база спинов OSD (реф. KOsdSpinBase : KFrame, ctor @0x485570, sizeof 0x48).
// АБСТРАКТНАЯ: расширяет vtable тремя ЧИСТО виртуальными слотами (+0x210 ConfirmKeyActImpl,
// +0x218 SubValue, +0x220 AddValue — в реф. это __cxa_pure_virtual).
//
// Единственное собственное поле — bool m_gotFocusIn @+0x42 (влезает в хвостовой паддинг
// KFrame). Смысл: Confirm по строке-спину РАБОТАЕТ КАК ТУМБЛЕР — первый вход «захватывает»
// спин (дальше Up/Down меняют ЗНАЧЕНИЕ), второй отпускает (Up/Down снова листают меню).
// Само меню (KOsdSubMenu::Up/DownKeyAct) перед перемещением курсора спрашивает GotFocusIn().
//
// Чистый UI, device-seam отсутствует. В нашем порте этот класс раньше был СПЛЮЩЕН в QFrame
// (см. KOsdSpin) — тумблер фокуса при этом терялся.
class KOsdSpinBase : public KFrame
{
    Q_OBJECT
public:
    explicit KOsdSpinBase(QWidget *parent = nullptr);

    bool GotFocusIn() const override { return m_gotFocusIn; }   // реф. @0x485620
    void ReleaseFocus() override { m_gotFocusIn = false; }      // реф. @0x4856a8 (визуала нет)

    // Реф. @0x4855b0: m_gotFocusIn ^= 1 → Focused() либо Selected() → ConfirmKeyActImpl(index).
    void ConfirmKeyAct(int index) override;
    // Реф. @0x485628 / @0x485668: работают ТОЛЬКО при захваченном фокусе.
    void UpKeyAct() override;
    void DownKeyAct() override;

protected:
    // Реф. чисто виртуальные слоты подкласса.
    virtual void ConfirmKeyActImpl(int index) = 0;
    virtual void SubValue() = 0;
    virtual void AddValue() = 0;

private:
    bool m_gotFocusIn = false;   // +0x42
};
