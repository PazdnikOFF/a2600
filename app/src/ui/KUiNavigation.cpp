#include "KUiNavigation.h"

#include <QWidget>

#include <functional>
#include <utility>

#include "sys/KAccount.h"
#include "sys/KProjectSet.h"


#include "KColdlightAdjust.h"
#include "KControlInfo.h"
#include "KDeviceInfo.h"
#include "KEndoScopeControl.h"
#include "KEndoScopeSN.h"
#include "KExamListCancelDlg.h"
#include "KExamListSetupDlg.h"
#include "KGeneralSetDlg.h"
#include "KImportRules.h"
#include "KLogView.h"
#include "KPatientListAddDlg.h"
#include "KPatientListEditDlg.h"
#include "KPatientListSetupDlg.h"
#include "KProcessorControl.h"
#include "KProcessorSN.h"
#include "KRecordCase.h"
#include "KSysDicom.h"
#include "KSysPrinter.h"
#include "KThesaurusSaveUi.h"
#include "KUpdateAction.h"
#include "KUpdateMng.h"
#include "KUpdatePrepare.h"
#include "KUserSrvSet.h"

namespace nav {

void CenterOn(QWidget *dlg, QWidget *ref)
{
    // Реф.: координаты берутся из crect обоих виджетов, деление ЦЕЛОЧИСЛЕННОЕ.
    if (!dlg || !ref)
        return;
    dlg->move((ref->width() - dlg->width()) / 2, (ref->height() - dlg->height()) / 2);
}

// Гейт «включён хотя бы один режим контроля машины» (реф. четыре метода KControlProc).
static std::function<bool()> g_machineControl;
void SetMachineControlProvider(std::function<bool()> fn) { g_machineControl = std::move(fn); }
bool MachineControlEnabled() { return g_machineControl ? g_machineControl() : false; }

// Роль текущего пользователя (реф. GetKAccount()->CurrentRole()). Гейты сравнивают
// ЧИСЛЕННО (`role > 1`, `role > 3`), поэтому и здесь число, а не enum.
static int CurrentRoleValue()
{
    return static_cast<int>(KAccount::GetInstance().CurrentRole());
}

} // namespace nav

// --- Без гейтов ------------------------------------------------------------------
void OpenColdlightAdjustDlg() { KColdlightAdjust dlg;  dlg.exec(); }   // реф. @0x7488a8
void OpenEndoControlDlg()     { KEndoScopeControl dlg; dlg.exec(); }   // реф. @0x7402a0
void OpenEndoScopeSNSetDlg()  { KEndoScopeSN dlg;      dlg.exec(); }   // реф. @0x742818
void OpenImportRulesDlg()     { KImportRules dlg;      dlg.exec(); }   // реф. @0x818e80
void OpenSysPrintDlg()        { KSysPrinter dlg;       dlg.exec(); }   // реф. @0x79c448
void OpenSystemDICOMSetDlg()  { KSysDicom dlg;         dlg.exec(); }   // реф. @0x5c2318
void OpenSystemGeneralSetDlg(){ KGeneralSetDlg dlg;    dlg.exec(); }   // реф. @0x5eb030

// --- С центрированием (⚠️ parent диалога = nullptr, аргумент только для геометрии) ---
void OpenExamListSetupDlg(QWidget *ref)
{
    KExamListSetupDlg dlg;  nav::CenterOn(&dlg, ref);  dlg.exec();     // реф. @0x7fa000
}
void OpenPatientListAddDlg(QWidget *ref)
{
    KPatientListAddDlg dlg; nav::CenterOn(&dlg, ref);  dlg.exec();     // реф. @0x7c2be0
}
void OpenPatientListSetupDlg(QWidget *ref)
{
    KPatientListSetupDlg dlg; nav::CenterOn(&dlg, ref); dlg.exec();    // реф. @0x7dc830
}
void OpenExamListCancelDlg(const QString &examKey, QWidget *ref)
{
    // Реф. @0x7ec5e0: ключ записи уходит ПЕРВЫМ аргументом ctor, parent = nullptr.
    KExamListCancelDlg dlg(examKey);  nav::CenterOn(&dlg, ref);  dlg.exec();
}
void OpenPatientListEditDlg(const QString &patientKey, QWidget *ref)
{
    KPatientListEditDlg dlg(patientKey); nav::CenterOn(&dlg, ref); dlg.exec();   // реф. @0x7cf4c0
}

// --- Аргумент = настоящий Qt-родитель ---------------------------------------------
void OpenUpdateAction(QWidget *parent)
{
    KUpdateAction dlg(parent);  dlg.exec();       // реф. @0x6e6498
}
int OpenUpdatePrepare(QWidget *parent)
{
    // Реф. @0x6e2338: наружу идёт поле +0x58 (m_state), а НЕ код exec(). Значение 15 ставит
    // KUpdatePrepare::UpdateProgressDec @0x6e1940 («распаковка закончена, идём прошивать»);
    // закрытие без распаковки оставляет 0 и обрывает цепочку.
    KUpdatePrepare dlg(parent);
    dlg.exec();
    return dlg.State();
}

int OpenUpdateMng()
{
    // Реф. @0x716c20: гейт роли, затем невидимый роутер-диалог; наружу — его поле +0x58.
    if (nav::CurrentRoleValue() <= 1)
        return 0;
    KUpdateMng dlg;
    return dlg.exec();
}

// --- Гейты -------------------------------------------------------------------------
void OpenControlInfoDlg()
{
    // Реф. @0x707e58: диалог только если включён ХОТЯ БЫ ОДИН из ЧЕТЫРЁХ режимов
    // контроля (IsStartTimeMc || IsStartEndoMc || IsStartEndoUseTimeCtrl ||
    // IsStartMatchProcessorCtrl). Все четыре — device (KControlProc читает ini прибора и
    // эндоскоп), поэтому в порте гейт вынесен в инъектируемый провайдер: по умолчанию
    // выключено ⇒ диалог не открывается (как на приборе без контроля).
    if (!nav::MachineControlEnabled())
        return;
    KControlInfo dlg;
    dlg.exec();
}

void OpenDeviceInfoDlg()
{
    // Реф. @0x7397e8: `if (KProjectSet::IsHideQRCode()) return;`
    if (KProjectSet::GetInstance().IsHideQRCode())
        return;
    KDeviceInfo dlg;
    dlg.exec();
}

void OpenLogViewDlg()
{
    if (nav::CurrentRoleValue() <= 1)   // реф. `cmp #1; b.le` → выход
        return;
    KLogView dlg;
    dlg.exec();
}

void OpenProcessorControlDlg()
{
    if (nav::CurrentRoleValue() <= 1)
        return;
    KProcessorControl dlg;
    dlg.exec();
}

bool OpenRecordCase()
{
    // Реф. @0x734348: роль > 3, иначе false; возврат — поле +0x58 диалога != 0.
    if (nav::CurrentRoleValue() <= 3)
        return false;
    KRecordCase dlg;
    dlg.exec();
    return dlg.IsRecordStarted();
}

// --- Диспетчер ----------------------------------------------------------------------
void OpenUserSrvSetDlg()
{
    // Реф. @0x70e000: у оператора (роль <= 1) вместо сервисного меню открывается
    // информация об устройстве — это НЕ просто выход, а tail-call.
    if (nav::CurrentRoleValue() <= 1) {
        OpenDeviceInfoDlg();
        return;
    }
    KUserSrvSet dlg;
    dlg.exec();
    // Реф.: после закрытия читается KUserSrvSet::m_eJumpModule (+0x68) и открывается
    // соответствующий экран (значения _KModuleId проверены по слотам-источникам).
    switch (dlg.JumpModule()) {
    case KUserSrvSet::JumpScopeInfo:       /* реф. 6 → OpenKScopeInfoEdit(false) */ break;
    case KUserSrvSet::JumpLogView:         OpenLogViewDlg();          break;   // 9
    case KUserSrvSet::JumpProcessorCtrl:   OpenProcessorControlDlg(); break;   // 10
    case KUserSrvSet::JumpEndoControl:     OpenEndoControlDlg();      break;   // 11
    case KUserSrvSet::JumpUpdateMng:       OpenUpdateMng();           break;   // 13
    case KUserSrvSet::JumpColdlightAdjust: OpenColdlightAdjustDlg();  break;   // 16
    case KUserSrvSet::JumpVideoCal:        /* реф. 17 → OpenVideoCal() */      break;
    default:                                                          break;
    }
}

// --- Прочее --------------------------------------------------------------------------
QString OpenProcessorSNSetDlg()
{
    // Реф. @0x6fff40: код exec() ИГНОРИРУЕТСЯ, наружу отдаётся GetSetSN().
    // ⚠️ В прошивке эта функция не вызывается ниоткуда (мёртвая точка входа).
    KProcessorSN dlg;
    dlg.exec();
    return dlg.GetSetSN();
}

void OpenThesaurusSaveDlg(const QString &s1, const QString &s2, int scopeClass)
{
    // Реф. @0x4e3f40: единственная из всех, кто открывает через KDialog::DoModal(),
    // а не exec(). Аргументы уходят в ctor диалога; у нас ctor их пока не принимает —
    // передаём через сеттер (тексты диагноза/раздела).
    KThesaurusSaveUi dlg;
    dlg.SetSaveContext(s1, s2, scopeClass);
    dlg.DoModal();
}
