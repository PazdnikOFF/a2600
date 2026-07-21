#pragma once

#include "KFullScreenDialog.h"

class QGroupBox;
class QStackedWidget;
class QLabel;
class QPushButton;
class QModelIndex;
class KPatientListWidget;
class KPatientListViewUi;
class KPatientListOptUi;
class KExamListViewUi;
class KExamListOptUi;
class KDicomQueueViewUi;
class KDicomQueueOptUi;

// Оболочка управления пациентами (реф. KPatientManagmentUi @ctor 0x7a6ea0, base
// KFullScreenDialog, setupUi @0x7a75f8, 1920×1080). КАПСТОУН — собирает семью patient/exam/
// dicom из РЕАЛЬНЫХ портированных вью+тулбаров. Две ПАРАЛЛЕЛЬНЫЕ QStackedWidget: слева
// grp_opt (nav-меню KPatientListWidget + HLine + stackedWidget_opt тулбаров + disk-метка +
// btn_exit), справа stackedWidget_tableview (вью). Выбор строки nav переключает ОБА стека.
// Ленивые Init*View-фабрики создают вью+тулбар и addWidget в оба стека.
// Базу KFullScreenDialog подставляем KDialog(KDLG_FULLSCREEN). DEVICE-STUB: disk-поток/IPC.
class KPatientManagmentUi : public KFullScreenDialog
{
    Q_OBJECT
public:
    explicit KPatientManagmentUi(QWidget *parent = nullptr);

    enum E_VIEW { E_PATIENT = 0, E_EXAM = 1, E_DICOM = 2 };
    void EntryView(E_VIEW v);   // реф.: setCurrentRow(v)+SwitchPage

    KPatientListViewUi *PatientView();   // порт-хук: гарантирует+возвращает (для инъекции провайдера)
    KExamListViewUi *ExamView();
    KDicomQueueViewUi *DicomView();

private slots:
    void ItemClicked(const QModelIndex &idx);   // реф. @: выбор строки → SwitchPage
    void SwitchPage();
    void ExitCurrentView();

private:
    void buildUi();
    void InitListWidgetItem();
    void InitPatientlistView();   // реф. @0x7a20d8
    void InitExamlistView();
    void InitDicomQueueView();

    QGroupBox *m_grpOpt = nullptr;
    KPatientListWidget *m_listWidget = nullptr;
    QStackedWidget *m_stackOpt = nullptr;
    QStackedWidget *m_stackView = nullptr;
    QLabel *m_labelDisk = nullptr;
    QPushButton *m_btnExit = nullptr;

    KPatientListViewUi *m_patientView = nullptr;   // +0xB8
    KPatientListOptUi *m_patientOpt = nullptr;     // +0xA0
    KExamListViewUi *m_examView = nullptr;         // +0xC0
    KExamListOptUi *m_examOpt = nullptr;           // +0xA8
    KDicomQueueViewUi *m_dicomView = nullptr;      // +0xC8
    KDicomQueueOptUi *m_dicomOpt = nullptr;        // +0xB0
};
