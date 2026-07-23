#pragma once

#include <QFrame>
#include <QPoint>
#include <QString>

class KOsdSubMenu;

// Payload-база для строк OSD-подменю (реф. KFrame : QFrame, ctor @0x47f490, sizeof 0x48).
// Тонкий QFrame + виртуальный интерфейс навигации аппаратными клавишами; конкретные строки
// (KOsdLabel/KOsdStatusLabel/KOsdSingleSelectLabel/KOsdSpinBase→KOsdSpin02/KOsdReturnLabel)
// его переопределяют. Это тип аргумента KOsdSubMenu::AddItem(KFrame*).
//
// ⚠️ РАСКЛАДКА ПОЛЕЙ ВЫВЕРЕНА ДИЗАСМОМ (правка 2026-07-23; раньше тут были фиктивные
// m_ptr/m_flag): +0x30 _KPoint m_startPoint (8 байт = 2×int, пишется одним `str x1`),
// +0x38 KOsdSubMenu* m_locatedMenu, +0x40 bool m_greyed, +0x41 bool m_selected.
// Ctor обнуляет ровно [+0x38] и полуслово [+0x40] (то есть оба bool).
//
// Слоты vtable (сверены вызовами из KOsdSubMenu): 0x1a0 Focused, 0x1a8 Selected,
// 0x1b0 UnSelected, 0x1b8/0x1c0 Checked/UnChecked (ICF-склеены), 0x1c8 GotFocusIn,
// 0x1d0 ReleaseFocus, 0x1d8 DownKeyAct, 0x1e0 UpKeyAct, 0x1e8 ConfirmKeyAct(int),
// 0x1f0 IsSingleSelectLabel, 0x1f8 SetIntValue, 0x200 SetDoubleValue, 0x208 SetStringValue.
// ⚠️ Аргумент ConfirmKeyAct — НЕ код клавиши, а ИНДЕКС выделенной строки (m_selectedIndex).
class KFrame : public QFrame
{
    Q_OBJECT
public:
    explicit KFrame(QWidget *parent = nullptr);

    // Виртуальный интерфейс навигации (в базе — no-op, реф. общий стаб @0x47f530).
    virtual void UpKeyAct() {}
    virtual void DownKeyAct() {}
    virtual void ReleaseFocus() {}
    virtual void UnChecked() {}
    virtual void Checked() {}                                   // реф. single-select чек-иконка
    virtual void ConfirmKeyAct(int index) { Q_UNUSED(index); }  // индекс строки, НЕ код клавиши
    virtual bool IsSingleSelectLabel() const { return false; }  // реф. @0x47f538 → false
    virtual bool GotFocusIn() const { return false; }           // реф. @0x47f538 → false

    // ⚠️ У БАЗЫ ЭТО НЕ ЗАГЛУШКИ (было ошибкой порта): реф. @0x47f580/@0x47f678/@0x47f780
    // реально ставят stylesheet (строки прочитаны из .rodata дословно, вместе с квирком
    // вендора «rgb(0,  205,209)» — ДВА пробела) и ведут m_selected.
    virtual void Focused();      // реф. @0x47f580: рамка #009EA7 + бирюзовый текст
    virtual void Selected();     // реф. @0x47f678: рамка + светло-серый текст, m_selected=true
    virtual void UnSelected();   // реф. @0x47f780: без рамки, m_selected=false

    virtual void SetIntValue(int v) { Q_UNUSED(v); }                 // реф. vt+0x1f8
    virtual void SetDoubleValue(double v) { Q_UNUSED(v); }           // реф. vt+0x200
    virtual void SetStringValue(const QString &v) { Q_UNUSED(v); }   // реф. vt+0x208

    // Невиртуальные аксессоры (реф. @0x47f558/0x47f560/0x47f570/0x47f578/0x47f880).
    void SetStartPoint(const QPoint &p) { m_startPoint = p; }
    QPoint GetStartPoint() const { return m_startPoint; }
    void SetLocatedMenu(KOsdSubMenu *m) { m_locatedMenu = m; }
    KOsdSubMenu *GetLocatedMenu() const { return m_locatedMenu; }
    bool IsGreyed() const { return m_greyed; }        // реф. ldrb [+0x40]
    void SetGreyed(bool g) { m_greyed = g; }          // реф. strb [+0x40]
    bool IsSelected() const { return m_selected; }    // реф. ldrb [+0x41] (читает UpdateUI)
    void UpdateUI();                                  // реф. @0x47f888: selected ? Selected : UnSelected

private:
    QPoint m_startPoint;                      // +0x30 (реф. _KPoint)
    KOsdSubMenu *m_locatedMenu = nullptr;     // +0x38
    bool m_greyed = false;                    // +0x40
    bool m_selected = false;                  // +0x41
};
