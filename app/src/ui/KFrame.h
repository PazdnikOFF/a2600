#pragma once

#include <QFrame>
#include <QPoint>

// Payload-база для ячеек OSD-подменю (реф. KFrame : QFrame, ctor @0x47f490). Тонкий QFrame,
// добавляющий ВИРТУАЛЬНЫЙ интерфейс (по умолч. no-op) — конкретные виджеты (KOsdMenuCell/
// KOsdSpin) его переопределяют. Это тип аргумента KOsdSubMenu::AddItem(KFrame*).
class KFrame : public QFrame
{
    Q_OBJECT
public:
    explicit KFrame(QWidget *parent = nullptr);

    // Виртуальный интерфейс (реф. — общий пустой стаб @0x47f530 + прочие no-op).
    virtual void UpKeyAct() {}
    virtual void DownKeyAct() {}
    virtual void ReleaseFocus() {}
    virtual void UnChecked() {}
    virtual void Checked() {}                                    // реф. single-select чек-иконка
    virtual void ConfirmKeyAct(int value) { Q_UNUSED(value); }  // реф. @0x582bf0: DEVICE-seam в подклассе
    virtual bool IsSingleSelectLabel() const { return false; }  // реф. @0x47f538 база → false
    virtual void Focused() {}
    virtual void Selected() {}
    virtual void UnSelected() {}
    virtual void SetValue(int v) { Q_UNUSED(v); }
    virtual void SetDoubleValue(double v) { Q_UNUSED(v); }
    virtual void SetStartPoint(const QPoint &p) { Q_UNUSED(p); }
    virtual bool IsGreyed() const { return false; }
    virtual bool GotFocusIn() const { return false; }
    virtual void UpdateUI() {}

private:
    void *m_ptr = nullptr;   // +0x38
    short m_flag = 0;        // +0x40
};
