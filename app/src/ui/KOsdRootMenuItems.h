#pragma once

#include "KOsdMenuCell.h"

// Ячейки КОРНЕВОГО OSD-меню (реф. строятся в KOsdMenu::InitWidget @0x479c70). Тонкие
// KOsdMenuCell-подклассы: ctor = SetIcons + SetTitle; ConfirmAct открывает своё подменю/close.

// Реф. KOsdMenuCellGreyedWhenCameraDisconnected @0x482250: KOsdMenuCell + connect(GetCamera()::
// CameraStatusChanged → грей-себя). DEVICE-seam (камера) в порте опущен → CheckGreyedCondition
// база false (камера считается подключённой). Тип-маркер для «серых при отключении» ячеек.
class KOsdMenuCellGreyedWhenCameraDisconnected : public KOsdMenuCell
{
    Q_OBJECT
public:
    explicit KOsdMenuCellGreyedWhenCameraDisconnected(QWidget *parent = nullptr)
        : KOsdMenuCell(parent) {}
    // Реф.: серость по KCamera::CameraStatusChanged (device). В порте — не серо (камера on).
    bool CheckGreyedCondition() override { return false; }
};

// #1 TR_Mode → KIrisMenu (реф. OpenIrisMenu).
class KIrisItem : public KOsdMenuCellGreyedWhenCameraDisconnected
{
    Q_OBJECT
public:
    explicit KIrisItem(QWidget *parent = nullptr);
    void ConfirmAct() override;
};

// #3 TR_IParameters → KImageProcessingMenu (реф. OpenImageProcessingMenu; подменю DEFERRED).
class KimageProcesItem : public KOsdMenuCellGreyedWhenCameraDisconnected
{
    Q_OBJECT
public:
    explicit KimageProcesItem(QWidget *parent = nullptr);
    void ConfirmAct() override;
};

// #5 TR_Fnctn → KFeatureMenu (реф. OpenFeatureMenu; подменю DEFERRED).
class KFeaturesItem : public KOsdMenuCellGreyedWhenCameraDisconnected
{
    Q_OBJECT
public:
    explicit KFeaturesItem(QWidget *parent = nullptr);
    void ConfirmAct() override;
};

// #7 TR_Camera → KSnMenu (реф. OpenSnMenu).
class KCameraInfoItem : public KOsdMenuCellGreyedWhenCameraDisconnected
{
    Q_OBJECT
public:
    explicit KCameraInfoItem(QWidget *parent = nullptr);
    void ConfirmAct() override;
};

// #2 TR_Rcd/TR_RStop → старт/стоп записи (реф. KRecordItem @0x486160, sizeof 0x68, своих
// полей нет). Единственная ячейка, у которой ИКОНКИ И ЗАГОЛОВОК МЕНЯЮТСЯ на лету:
//   status 0 (не пишем)   → video_normal/video_select/video_select_grey.png, title TR_Rcd
//   status 2 (идёт запись)→ recording_normal/recording_select/recording_select.png (серая
//                           иконка та же, что выбранная!), title TR_RStop
// Ctor подписывается на KSystemStatus::SystemStatusChange и KUsbDevice::UsbStatusChange.
// SystemStatusChangeAct(type,value): фильтр `type != 7` (⚠️ именно 7 = ST_Record) →
// UpdateRecordStatus(value) + UpdateUI. UsbStatusChangeAct: аргумент ИГНОРИРУЕТ, просто
// пересчитывает серость. CheckGreyedCondition: камера не готова ИЛИ USB отключён.
// HasSubMenu() → false. ConfirmAct — почти целиком device-seam (см. .cpp).
class KRecordItem : public KOsdMenuCellGreyedWhenCameraDisconnected
{
    Q_OBJECT
public:
    explicit KRecordItem(QWidget *parent = nullptr);

    void ConfirmAct() override;
    bool CheckGreyedCondition() override;   // реф. @0x486338
    bool HasSubMenu() const { return false; }   // реф. @0x486330

    // Код возврата корневого меню при старте записи (реф. done(2) @0x868484).
    static const int ReturnCodeForRecording = 2;

public slots:
    void UpdateRecordStatus(int status);          // реф. @0x485de0 — иконки+заголовок
    void SystemStatusChangeAct(int type, int v);  // реф. @0x486360 — фильтр type == 7
    void UsbStatusChangeAct(int state);           // реф. @0x485db8

signals:
    void recordToggleRequested();   // порт: замена SendToMainCtrl(2)/done(2)
};

// #4 TR_Ext → close (реф. KDialog::close, plain KOsdMenuCell — 2 иконки, не сереет).
class KExitItem : public KOsdMenuCell
{
    Q_OBJECT
public:
    explicit KExitItem(QWidget *parent = nullptr);
    void ConfirmAct() override;
};

// #6 TR_Button → KButtonDefinitionMenu (реф. OpenButtonDefinitionMenu, plain KOsdMenuCell).
class KButtonDefineItem : public KOsdMenuCell
{
    Q_OBJECT
public:
    explicit KButtonDefineItem(QWidget *parent = nullptr);
    void ConfirmAct() override;
};
