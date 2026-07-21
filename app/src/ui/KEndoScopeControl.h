#pragma once

#include "ui/KDialog.h"

// Диалог управления эндоскопом/машинного контроля (реф. KEndoScopeControl : KDialog,
// ctor @0x740130, Ui_KEndoScopeControl::setupUi @0x740d28). UI-порт. Сиблинг
// KProcessorControl, открывается из KUserSrvSet(Endo). Немодальный, 640×480,
// SetKStyle(W460), титул TR_EControl. Две группы: groupBox(TR_UOEControl) — срок/остаток/
// число использований (截止日期/剩余天数/次数 device) + 开启/解除管控/TR_IAuthorization;
// groupBox_2(TR_CPControl) — число совместимых процессоров (TR_CPAmount:/Num device) +
// TR_Vw + 开启/解除管控/TR_IAuthorization. Снизу Exit. Только метки+кнопки, stock Qt.
//
// DEVICE в порт не тянется: ChangeUseTimesCtrlState/ImportDelayLic/ChangeMatchProCtrlState/
// ViewMatchProList/ImportMatchProLic, KControlProc(GetEndoRemainTimes/GetMatchProcessorList/
// IsStart*Ctrl), RecMainCtrl (GetMainCtrlThread) — заглушки. Exit→close.
class KEndoScopeControl : public KDialog
{
    Q_OBJECT
public:
    explicit KEndoScopeControl(QWidget *parent = nullptr);

private:
    void setupUi();
};
