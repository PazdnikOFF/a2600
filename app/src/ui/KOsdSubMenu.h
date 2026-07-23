#pragma once

#include "KOsdMenu.h"
#include "KOsdSpin.h"
#include "KOsdSpin02.h"
#include "KOsdReturnLabel.h"
#include "KOsdDoubleSpin.h"
#include "KOsdLabel.h"
#include "KOsdStatusLabel.h"
#include "KOsdSingleSelectLabel.h"
#include <QList>
#include <QPoint>

class QVBoxLayout;
class QListWidget;
class QModelIndex;
class QKeyEvent;

// OSD-подменю, хостящее спины/метки (реф. KOsdSubMenu : KOsdMenuBase, ctor @0x47b4f0
// (QWidget*, bool bAddReturnBtn)). ХОСТИТ портированные KOsdSpin/KOsdDoubleSpin через
// AddItem-фабрики от конфигов (спин-строка = KOsdSpin02, единственный живой спин прошивки;
// строка «назад» = KOsdReturnLabel, дописывается в InitWidget). Шелл: QVBoxLayout(0) + QListWidget(NoFocus,
// translucent, «color: rgb(160,160,160)», item margin 4px), фикс-ширина 270. InitWidget:
// height=count*44, клип к 1080. Навигация 1:1 с реф.: Up/Down сначала спрашивают у текущей
// строки GotFocusIn() (захваченный спин ест клавиши сам), заворачивание курсора живёт в
// RefreshSelectedItem; Left/Escape = отпустить спин либо закрыть меню. SetValue(row,…)
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
    void AddItem(const KOsdStatusLabelConfig &cfg);  // реф. → KOsdStatusLabel
    void AddItem(const KOsdSingleSelectLabelConfig &cfg);  // реф. → KOsdSingleSelectLabel (action=null)
    void AddAReturnBtnInTheEndIfNeeded();            // реф. @0x47bf60 → KOsdReturnLabel в конец
    KFrame *GetSelectedMenuItem() const;             // реф. @0x47ac88
    void InitWidget(const QPoint &pos);              // реф. @0x47bfe0: размер/позиция/клип 1080
    void SetValue(int row, int value);               // фан-аут в KOsdSpin
    void SetValue(int row, double value);            // фан-аут в KOsdDoubleSpin

    void InitCheckedItem(int idx);                   // реф. @0x47b078: коммит + refresh (single-select)
    void RefreshCheckedItem();                       // реф. @0x47af90: чек/расчек иконок

protected:
    void keyPressEvent(QKeyEvent *e) override;   // реф. @0x47b218: коды клавиш → действия

public slots:
    void UpKeyAct();
    void DownKeyAct();
    void ConfirmKeyAct();
    void ItemClicked(const QModelIndex &idx);

private:
    void RefreshSelectedItem();   // реф. @0x47ae30 (сам заворачивает курсор)

    QVBoxLayout *m_layout = nullptr;
    QListWidget *m_listWidget = nullptr;
    QList<QWidget *> m_items;    // +0x88 (в реф. KFrame*)
    bool m_bAddReturnBtn = false;   // +0x60
    bool m_returnBtnAdded = false;  // +0x61 (защёлка «строка назад уже добавлена»)
    int m_selectedIndex = 0;     // +0x90
    int m_checkedIndex = -1;     // +0x94 (single-select: закоммиченный выбор)
    QPoint m_pos;                // +0x70/0x74
};
