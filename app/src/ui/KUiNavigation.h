#pragma once

#include <QString>

#include <functional>

class QWidget;

// СЛОЙ НАВИГАЦИИ (реф. свободные функции `Open*`, ~46 штук; здесь 22 портированные).
// В прошивке каждая живёт рядом со «своим» диалогом; у нас собраны в один модуль, чтобы
// не размазывать одинаковую обвязку по 22 файлам — поведение 1:1.
//
// Общая схема всех Open*: `new` диалога на куче → (иногда) центрирование по переданному
// виджету → exec()/DoModal() → чтение результата → delete. Утечек в реф. нет ни в одной
// функции. Часть функций — ГЕЙТЫ: при непройденном условии диалог не создаётся вовсе.
//
// ⚠️ Аргумент `QWidget* pParent` у большинства функций — НЕ Qt-родитель: диалог строится с
// parent = nullptr, а аргумент нужен только для центрирования (реф. читает crect обоих).
namespace nav {

// Центр по «родителю» (реф. move((p->width()-d->width())/2, (p->height()-d->height())/2)).
void CenterOn(QWidget *dlg, QWidget *ref);

// Гейт OpenControlInfoDlg: реф. спрашивает четыре метода KControlProc (device). В порте —
// инъектируемый провайдер; по умолчанию false ⇒ диалог не открывается.
void SetMachineControlProvider(std::function<bool()> fn);
bool MachineControlEnabled();

} // namespace nav

// --- Без гейтов, модальные, результат игнорируется --------------------------------
void OpenColdlightAdjustDlg();          // реф. @0x7488a8 → KColdlightAdjust
void OpenEndoControlDlg();              // реф. @0x7402a0 → KEndoScopeControl
void OpenEndoScopeSNSetDlg();           // реф. @0x742818 → KEndoScopeSN
void OpenImportRulesDlg();              // реф. @0x818e80 → KImportRules
void OpenSysPrintDlg();                 // реф. @0x79c448 → KSysPrinter
void OpenSystemDICOMSetDlg();           // реф. @0x5c2318 → KSysDicom
void OpenSystemGeneralSetDlg();         // реф. @0x5eb030 → KGeneralSetDlg

// --- Центрируются по переданному виджету (parent диалога = nullptr!) ---------------
void OpenExamListSetupDlg(QWidget *ref);                              // реф. @0x7fa000
void OpenPatientListAddDlg(QWidget *ref);                             // реф. @0x7c2be0
void OpenPatientListSetupDlg(QWidget *ref);                           // реф. @0x7dc830
void OpenExamListCancelDlg(const QString &examKey, QWidget *ref);     // реф. @0x7ec5e0
void OpenPatientListEditDlg(const QString &patientKey, QWidget *ref); // реф. @0x7cf4c0

// --- Аргумент реально становится Qt-родителем (только эти две) ---------------------
void OpenUpdateAction(QWidget *parent);        // реф. @0x6e6498 → KUpdateAction
int  OpenUpdatePrepare(QWidget *parent);       // реф. @0x6e2338 → KUpdatePrepare, возврат поля +0x58

// --- Гейты ------------------------------------------------------------------------
void OpenControlInfoDlg();      // реф. @0x707e58: открывается, только если включён ХОТЯ БЫ
                                // один из четырёх режимов контроля (KControlProc)
void OpenDeviceInfoDlg();       // реф. @0x7397e8: гейт `if (IsHideQRCode()) return;`
void OpenLogViewDlg();          // реф. @0x713200: роль > 1
void OpenProcessorControlDlg(); // реф. @0x73b9d8: роль > 1
bool OpenRecordCase();          // реф. @0x734348: роль > 3, иначе false; возврат поля +0x58

// --- Диспетчер: после закрытия сам открывает следующий экран -----------------------
void OpenUserSrvSetDlg();       // реф. @0x70e000: роль > 1, иначе tail-call OpenDeviceInfoDlg()

// --- Прочее -----------------------------------------------------------------------
QString OpenProcessorSNSetDlg();   // реф. @0x6fff40 — ⚠️ МЁРТВАЯ точка входа (0 вызовов)
void OpenThesaurusSaveDlg(const QString &s1, const QString &s2, int scopeClass);  // реф. @0x4e3f40 (DoModal)
