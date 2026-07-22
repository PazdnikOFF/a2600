#pragma once

#include "KDialog.h"
#include <QList>
#include <QPoint>

class QVBoxLayout;
class QListWidget;
class QModelIndex;
class KOsdMenuCell;

// База OSD-меню (реф. KOsdMenuBase : KDialog, ctor @0x47a780). Трекинг уровня меню через
// цепочку m_parentMenu (нет хранимого int). Device-хуки *Impl — пустые виртуалы (реф. ctor
// коннектит KUiMsgProxy/SystemStatus/Camera/VideoParam — опущено как device). keyPressEvent
// F12→ExitAllMenus. Навигация в реф. идёт через KUiMsgProxy-сигналы (не клавиши).
class KOsdMenuBase : public KDialog
{
    Q_OBJECT
public:
    explicit KOsdMenuBase(QWidget *parent = nullptr);

    KOsdMenuBase *ParentMenu() const { return m_parentMenu; }
    void SetParentMenu(KOsdMenuBase *m) { m_parentMenu = m; }
    int GetOsdMenuLevel() const;   // реф.: глубина по цепочке (root=1)

public slots:
    virtual void ExitAllMenus() { close(); }
    virtual void TryToCloseAllOsdMenus() { ExitAllMenus(); }
    virtual void CloseTheMostOutsideMenusForCameraDisconnected() {}

protected:
    void keyPressEvent(QKeyEvent *) override;   // F12 → ExitAllMenus

    // DEVICE-хуки (реф. виртуалы, база no-op).
    virtual void VideoParamChangeActImpl(int, int) {}
    virtual void SystemStatusChangeActImpl(int, int) {}
    virtual void CameraStatusChangedActImpl(int) {}
    virtual bool NeedToCloseWhenCameraDisconnected() const { return false; }

private:
    KOsdMenuBase *m_parentMenu = nullptr;   // +0x50
};

// Конкретное OSD-меню (реф. KOsdMenu : KOsdMenuBase, ctor @0x479ed0). QVBoxLayout(margins 0) +
// QListWidget (NoFocus, прозрачный), безрамочное translucent-окно. ХОСТИТ портированный
// KOsdMenuCell через AddItem @0x479740 (insertItem + setItemWidget). Навигация Up/Down/Confirm/
// клик → RefreshMenu (Select/UnSelect/Focus ячеек + SizeHintRole из геометрии ячейки).
// DEVICE-STUB: InitWidget-список конкретных ячеек (KIrisItem/KRecordItem/…) — не портируется,
// ячейки добавляет вызывающий через AddItem.
class KOsdMenu : public KOsdMenuBase
{
    Q_OBJECT
public:
    explicit KOsdMenu(QWidget *parent = nullptr);

    void AddItem(KOsdMenuCell *cell);   // реф. @0x479740: хостинг ячейки
    void InitWidget();                  // реф. @0x479c70: сборка корневых ячеек

public slots:
    void UpKeyAct();        // idx-1 wrap
    void DownKeyAct();      // idx+1 wrap
    void ConfirmKeyAct();   // текущая ячейка → Focus + активация
    void ItemClicked(const QModelIndex &idx);

private:
    void RefreshMenu(int oldIdx);

    QVBoxLayout *m_layout = nullptr;
    QListWidget *m_listWidget = nullptr;
    int m_currentIndex = 0;           // +0x60
    QList<KOsdMenuCell *> m_cells;    // +0x68
    QPoint m_subPos = QPoint(0, 172); // +0x74/0x78
};
