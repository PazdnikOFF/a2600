#pragma once

#include <QFrame>
#include <QPoint>
#include <QString>

class QLabel;

// Ячейка OSD-меню (реф. KOsdMenuCell @ctor 0x4812a8, base QFrame — НЕ KOsdSpinBase/KFrame).
// UI-порт РЕАЛЬНОГО кастом-виджета. Отличия от KOsdSpin: derives напрямую от QFrame, БЕЗ
// SendToMainCtrl/сигналов (чисто визуальная), навигация — во владеющем KOsdMenu (QListWidget
// setItemWidget). Круглая иконка-кнопка + подпись + флаг «выбрано». Состояние — stylesheet-
// свап + смена fixed-size + свап пиксмапа иконки (без paintEvent).
//   Normal (UnSelect):  button_normal.png, radius 19, ширина 110, текст rgb(221,221,221)
//   Selected (Select):  button_select.png, radius 26, ширина 160, иконка 76, флаг виден
//   Focused (Focus):    button_select.png, radius 26, текст cyan rgb(0,205,209) (если не greyed)
// clicked() добавлен для Qt-идиоматики (в реф. взаимодействие шло через KOsdMenu-навигацию).
// DEVICE-STUB: KDisplayOption::GetOsdIconPixmap(name) → QPixmap из theme::asset("black/osd/…");
// CheckGreyedCondition() — virtual, база false (device-подкласс гейтит по камере).
class KOsdMenuCell : public QFrame
{
    Q_OBJECT
public:
    explicit KOsdMenuCell(QWidget *parent = nullptr);

    void SetTitle(const QString &title);   // реф. @0x481100
    // Имена иконок (реф. +0x30 selected/normal, +0x38 unselected, +0x40 greyed).
    void SetIcons(const QString &selected, const QString &unselected, const QString &greyed);

    void Select();     // реф. @0x4813f0
    void UnSelect();   // реф. @0x481680
    void Focus();      // реф. @0x481128
    void UpdateUI();   // реф. @0x481828: IsSelected()?Select():UnSelect()

    bool IsSelected() const { return m_selected; }
    bool IsGreyed() const { return m_greyed; }
    bool IsNotGreyed() const { return !m_greyed; }
    void SetGreyed(bool g) { m_greyed = g; }
    void UpdateGreyedFlag() { SetGreyed(CheckGreyedCondition()); }   // реф. @0x481248
    virtual bool CheckGreyedCondition() { return false; }            // реф. @0x480f98 (база)

    void SetSubWindowPosition(const QPoint &p) { m_subPos = p; }
    QPoint SubWindowPosition() const { return m_subPos; }
    void SetLocatedMenu(QObject *m) { m_menu = m; }
    QObject *LocatedMenu() const { return m_menu; }

    // Реф. подтверждение ячейки (открытие подменю / close) — virtual, база no-op; подклассы
    // корневого меню (KIrisItem/…) переопределяют. Зовётся из KOsdMenu::ConfirmKeyAct.
    virtual void ConfirmAct() {}

signals:
    void clicked();

protected:
    virtual bool showFlag() const { return true; }   // реф. virtual@vtable+0x1b0
    void mouseReleaseEvent(QMouseEvent *) override;

private:
    void setupUi();
    QPixmap osdIcon(const QString &name) const;   // theme::asset("black/osd/"+name)

    QFrame *m_frameFlag = nullptr;
    QFrame *m_frameIcon = nullptr;
    QLabel *m_labelFlag = nullptr;
    QLabel *m_labelIcon = nullptr;
    QLabel *m_labelTag = nullptr;
    QString m_iconSel;     // +0x30
    QString m_iconUnsel;   // +0x38
    QString m_iconGrey;    // +0x40
    QPoint m_subPos;       // +0x48
    QObject *m_menu = nullptr;   // +0x58
    bool m_greyed = false;   // +0x60
    bool m_selected = false; // +0x61
};
