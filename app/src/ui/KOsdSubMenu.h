#pragma once

#include "KOsdMenu.h"
#include "KOsdSpin.h"
#include "KOsdDoubleSpin.h"
#include "KOsdLabel.h"
#include <QList>
#include <QPoint>

class QVBoxLayout;
class QListWidget;
class QModelIndex;

// OSD-подменю, хостящее спины/метки (реф. KOsdSubMenu : KOsdMenuBase, ctor @0x47b4f0
// (QWidget*, bool bAddReturnBtn)). ХОСТИТ портированные KOsdSpin/KOsdDoubleSpin через
// AddItem-фабрики от конфигов (реф. строит KFrame-подклассы KOsdSpin02/… — в порте наши
// QFrame-спины, хостинг через QWidget*). Шелл: QVBoxLayout(0) + QListWidget(NoFocus,
// translucent, «color: rgb(160,160,160)», item margin 4px), фикс-ширина 270. InitWidget:
// height=count*44, клип к 1080. Навигация Up/Down/Confirm через курсор выбора. SetValue(row,…)
// — фан-аут значения в спин (device→UI). DEVICE-STUB: KUiMsgProxy nav-сигналы + SendToMainCtrl.
class KOsdSubMenu : public KOsdMenuBase
{
    Q_OBJECT
public:
    explicit KOsdSubMenu(QWidget *parent = nullptr, bool bAddReturnBtn = false);

    void AddItem(QWidget *w);                        // реф. @0x47b9c8: базовый хост
    void AddItem(const KOsdSpinConfig &cfg);         // реф. @0x47bd20 → KOsdSpin
    void AddItem(const KOsdDoubleSpinConfig &cfg);   // реф. @0x47be40 → KOsdDoubleSpin
    void AddItem(const KOsdLabelConfig &cfg);        // реф. @0x47baa0 → KOsdLabel
    void InitWidget(const QPoint &pos);              // реф. @0x47bfe0: размер/позиция/клип 1080
    void SetValue(int row, int value);               // фан-аут в KOsdSpin
    void SetValue(int row, double value);            // фан-аут в KOsdDoubleSpin

    void InitCheckedItem(int idx);                   // реф. @0x47b078: коммит + refresh (single-select)
    void RefreshCheckedItem();                       // реф. @0x47af90: чек/расчек иконок

public slots:
    void UpKeyAct();
    void DownKeyAct();
    void ConfirmKeyAct();
    void ItemClicked(const QModelIndex &idx);

private:
    void RefreshSelectedItem(int oldIdx);

    QVBoxLayout *m_layout = nullptr;
    QListWidget *m_listWidget = nullptr;
    QList<QWidget *> m_items;    // +0x88 (в реф. KFrame*)
    bool m_bAddReturnBtn = false;
    int m_selectedIndex = 0;     // +0x90
    int m_checkedIndex = -1;     // +0x94 (single-select: закоммиченный выбор)
    QPoint m_pos;                // +0x70/0x74
};
