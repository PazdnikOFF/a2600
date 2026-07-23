#include "KOsdRootMenuItems.h"
#include "sys/KSystemStatus.h"
#include "hal/KUsbDevice.h"

#include <QDialog>
#include "KIrisMenu.h"
#include "KSnMenu.h"
#include "KButtonDefinitionMenu.h"
#include "KImageProcessingMenu.h"
#include "KFeatureMenu.h"

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
    // Реф. OpenImageProcessingMenu → KImageProcessingMenu (портирован).
    (new KImageProcessingMenu(SubWindowPosition()))->show();
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
    // Реф. OpenFeatureMenu → KFeatureMenu (портирован).
    (new KFeatureMenu(SubWindowPosition()))->show();
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

// ── KRecordItem (реф. @0x486160) ─────────────────────────────────────────────
KRecordItem::KRecordItem(QWidget *parent)
    : KOsdMenuCellGreyedWhenCameraDisconnected(parent)
{
    // Реф. ctor: сразу подтягивает текущий статус записи, затем два коннекта.
    UpdateRecordStatus(KSystemStatus::GetInstance().RecordStatus());
    connect(&KSystemStatus::GetInstance(), &KSystemStatus::SystemStatusChange,
            this, &KRecordItem::SystemStatusChangeAct);
    // Реф. второй коннект — KUsbDevice::UsbStatusChange (device); у нас USB-объект без
    // сигнала, поэтому подписка опущена: серость пересчитывается при UpdateRecordStatus.
}

void KRecordItem::UpdateRecordStatus(int status)
{
    // Реф. @0x485de0: ровно две ветки (0 и 2), остальные значения игнорируются.
    if (status == 0) {
        SetIcons(QStringLiteral("video_select.png"), QStringLiteral("video_normal.png"),
                 QStringLiteral("video_select_grey.png"));
        SetTitle(tr("TR_Rcd"));
    } else if (status == 2) {
        // ⚠️ Серая иконка НАМЕРЕННО совпадает с выбранной (в реф. один и тот же адрес).
        SetIcons(QStringLiteral("recording_select.png"), QStringLiteral("recording_normal.png"),
                 QStringLiteral("recording_select.png"));
        SetTitle(tr("TR_RStop"));
    } else {
        return;   // реф.: прочие статусы не трогают ни иконки, ни заголовок
    }
    UpdateGreyedFlag();   // реф. хвост всех веток
}

void KRecordItem::SystemStatusChangeAct(int type, int v)
{
    // Реф. @0x486360: фильтр по коду 7 (ST_Record) — коды выверены дизасмом, см. KSystemStatus.h.
    if (type != KSystemStatus::ST_Record)
        return;
    UpdateRecordStatus(v);
    UpdateUI();
}

void KRecordItem::UsbStatusChangeAct(int state)
{
    // Реф. @0x485db8: аргумент ИГНОРИРУЕТСЯ — только пересчёт серости и перерисовка.
    Q_UNUSED(state);
    UpdateGreyedFlag();
    UpdateUI();
}

bool KRecordItem::CheckGreyedCondition()
{
    // Реф. @0x486338: KCamera::CameraIsNotReady() || KUsbDevice::IsUsbDisconnect().
    // Камера — device-seam (в порте считаем готовой), USB читаем из портированного KUsbDevice.
    return KUsbDevice::GetInstance()->IsUsbDisconnect();
}

void KRecordItem::ConfirmAct()
{
    // Реф. @0x486290, порядок гейтов строгий:
    //   уже пишем        → SendToMainCtrl(2) (стоп записи)          — device-seam
    //   камера не готова → ShowCameraStatus(6)                      — device-seam
    //   USB отключён     → KUiMsgProxy::NoUsbDevice()               — device-seam
    //   иначе            → LocatedMenu()->done(2) — меню закрывается кодом «идём писать».
    if (KSystemStatus::GetInstance().RecordStatus() != 0) {
        emit recordToggleRequested();
        return;
    }
    if (KUsbDevice::GetInstance()->IsUsbDisconnect())
        return;   // реф. NoUsbDevice() — сообщение пользователю (device)
    emit recordToggleRequested();
    if (auto *dlg = qobject_cast<QDialog *>(LocatedMenu()))
        dlg->done(ReturnCodeForRecording);
}
