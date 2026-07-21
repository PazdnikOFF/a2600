#pragma once

#include "ui/KDialog.h"

// Диалог прогресса операций с данными (реф. KProgressDlg : QDialog, ctor @0x451870,
// Ui_KProgressDlg::setupUi @0x452528). UI-порт. Реф.-база — QDialog (не KDialog), но чистый
// drop-in над KDialog. 512×278. Разблокирует KDataOprEventDeal (см. §10 блокеры).
// Фрейм umessage_frame_back (StyledPanel/Raised) → vbox: label_Text(TR_Prpng, AlignLeft|
// Bottom) + [lblCurrentProgress(TR_CFProgress) + progCurrentFile + lblTotalProgress
// (TR_TProgress) + progTotal] + спейсер + btn_Cancel(TR_Ccl, minW150, центр). Прогресс-бары
// с border-стилем, value 0. Все stock Qt, кастомов нет.
//
// DEVICE в порт не тянется: ToDlgMsgDispatcher-сигналы (SigUpdate*Progress/Label/Hide/
// ShowResultMsgBox/OneExamRecordUpdateFinish — кросс-поточные апдейты) — заглушки.
// Публичный API (Set*Progress/Set*Label/SetTitle/DoModal/IsCancel) — портируемый.
// btn_Cancel→close.
class KProgressDlg : public KDialog
{
    Q_OBJECT
public:
    explicit KProgressDlg(QWidget *parent = nullptr);

private:
    void setupUi();
};
