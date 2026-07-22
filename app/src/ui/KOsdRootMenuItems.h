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
