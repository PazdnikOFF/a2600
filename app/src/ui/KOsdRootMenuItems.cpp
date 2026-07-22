#include "KOsdRootMenuItems.h"
#include "KIrisMenu.h"
#include "KSnMenu.h"
#include "KButtonDefinitionMenu.h"

#include <QWidget>

// ── KIrisItem ────────────────────────────────────────────────────────────────
KIrisItem::KIrisItem(QWidget *parent)
    : KOsdMenuCellGreyedWhenCameraDisconnected(parent)
{
    SetIcons(QStringLiteral("iris_select.png"), QStringLiteral("iris_normal.png"),
             QStringLiteral("iris_select_grey.png"));
    SetTitle(tr("TR_Mode"));
}
void KIrisItem::ConfirmAct()
{
    // Реф. OpenIrisMenu(m_subPos, LocatedMenu). KIrisMenu портирован.
    (new KIrisMenu(SubWindowPosition()))->show();
}

// ── KimageProcesItem ─────────────────────────────────────────────────────────
KimageProcesItem::KimageProcesItem(QWidget *parent)
    : KOsdMenuCellGreyedWhenCameraDisconnected(parent)
{
    SetIcons(QStringLiteral("imageprocessing_select.png"), QStringLiteral("imageprocessing_normal.png"),
             QStringLiteral("imageprocessing_select_grey.png"));
    SetTitle(tr("TR_IParameters"));
}
void KimageProcesItem::ConfirmAct()
{
    // Реф. OpenImageProcessingMenu → KImageProcessingMenu — DEFERRED (video-param-hook), не открываем.
}

// ── KFeaturesItem ────────────────────────────────────────────────────────────
KFeaturesItem::KFeaturesItem(QWidget *parent)
    : KOsdMenuCellGreyedWhenCameraDisconnected(parent)
{
    SetIcons(QStringLiteral("features_select.png"), QStringLiteral("features_normal.png"),
             QStringLiteral("features_select_grey.png"));
    SetTitle(tr("TR_Fnctn"));
}
void KFeaturesItem::ConfirmAct()
{
    // Реф. OpenFeatureMenu → KFeatureMenu — DEFERRED (video-param-hook), не открываем.
}

// ── KCameraInfoItem ──────────────────────────────────────────────────────────
KCameraInfoItem::KCameraInfoItem(QWidget *parent)
    : KOsdMenuCellGreyedWhenCameraDisconnected(parent)
{
    SetIcons(QStringLiteral("equipment_select.png"), QStringLiteral("equipment_normal.png"),
             QStringLiteral("equipment_select_grey.png"));
    SetTitle(tr("TR_Camera"));
}
void KCameraInfoItem::ConfirmAct()
{
    // Реф. OpenSnMenu(m_subPos, LocatedMenu). KSnMenu портирован.
    (new KSnMenu(SubWindowPosition()))->show();
}

// ── KExitItem (plain KOsdMenuCell, 2 иконки) ─────────────────────────────────
KExitItem::KExitItem(QWidget *parent)
    : KOsdMenuCell(parent)
{
    // Реф. SetIcons(2): sel/unsel, без grey (не сереет) → grey = sel.
    SetIcons(QStringLiteral("exit_select.png"), QStringLiteral("exit_normal.png"),
             QStringLiteral("exit_select.png"));
    SetTitle(tr("TR_Ext"));
}
void KExitItem::ConfirmAct()
{
    // Реф. KDialog::close(LocatedMenu): закрыть корневое меню.
    if (QWidget *m = qobject_cast<QWidget *>(LocatedMenu()))
        m->close();
}

// ── KButtonDefineItem (plain KOsdMenuCell, 2 иконки) ─────────────────────────
KButtonDefineItem::KButtonDefineItem(QWidget *parent)
    : KOsdMenuCell(parent)
{
    SetIcons(QStringLiteral("button definition_select.png"),
             QStringLiteral("button definition_normal.png"),
             QStringLiteral("button definition_select.png"));
    SetTitle(tr("TR_Button"));
}
void KButtonDefineItem::ConfirmAct()
{
    // Реф. OpenButtonDefinitionMenu(m_subPos, LocatedMenu). KButtonDefinitionMenu портирован.
    (new KButtonDefinitionMenu(SubWindowPosition()))->show();
}
