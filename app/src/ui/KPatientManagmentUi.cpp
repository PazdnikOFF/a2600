#include "KPatientManagmentUi.h"
#include "KPatientListWidget.h"
#include "KPatientListViewUi.h"
#include "KPatientListOptUi.h"
#include "KExamListViewUi.h"
#include "KExamListOptUi.h"
#include "KDicomQueueViewUi.h"
#include "KDicomQueueOptUi.h"

#include <QGroupBox>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QListWidgetItem>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>

KPatientManagmentUi::KPatientManagmentUi(QWidget *parent)
    : KFullScreenDialog(parent, 2000)   // реф. KFullScreenDialog(parent, 2000) — KObject-ID шины
{
    // Реф. ctor @0x7a6ea0: KFullScreenDialog(2000) → setupUi → title → InitListWidgetItem →
    // InitHorizontalLine → std::thread InitDiskLabel (DEVICE) → InitConnect → SubscribeMsg.
    // Фуллскрин-стиль ставит база KFullScreenDialog (SetKStyle(FULLSCREEN)).
    SetTitle(tr("TR_PManagement"));
    buildUi();
    InitListWidgetItem();

    connect(m_listWidget, &QListWidget::clicked, this, &KPatientManagmentUi::ItemClicked);
    connect(m_btnExit, &QPushButton::clicked, this, &KPatientManagmentUi::ExitCurrentView);

    EntryView(E_PATIENT);   // стартовая страница
}

void KPatientManagmentUi::buildUi()
{
    setObjectName(QStringLiteral("KPatientManagmentUi"));
    QWidget *content = ContentArea();

    QHBoxLayout *h5 = new QHBoxLayout(content);
    h5->setObjectName(QStringLiteral("horizontalLayout_5"));
    h5->setContentsMargins(0, 42, 0, 0);
    h5->setSpacing(0);

    // ЛЕВО — grp_opt (280w): nav-меню + линия + стек тулбаров + disk + exit.
    m_grpOpt = new QGroupBox(content);
    m_grpOpt->setObjectName(QStringLiteral("grp_opt"));
    m_grpOpt->setFixedWidth(280);
    QVBoxLayout *v = new QVBoxLayout(m_grpOpt);
    v->setObjectName(QStringLiteral("verticalLayout"));
    v->setContentsMargins(13, 30, 13, 0);

    m_listWidget = new KPatientListWidget(m_grpOpt);
    m_listWidget->setFixedWidth(215);
    v->addWidget(m_listWidget, 0, Qt::AlignHCenter);

    QFrame *line = new QFrame(m_grpOpt);
    line->setObjectName(QStringLiteral("line"));
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setMinimumWidth(230);
    v->addWidget(line);

    m_stackOpt = new QStackedWidget(m_grpOpt);
    m_stackOpt->setObjectName(QStringLiteral("stackedWidget_opt"));
    m_stackOpt->setFixedWidth(271);
    m_stackOpt->setFocusPolicy(Qt::NoFocus);
    v->addWidget(m_stackOpt);

    v->addStretch();

    m_labelDisk = new QLabel(m_grpOpt);
    m_labelDisk->setObjectName(QStringLiteral("label_disk_vol"));
    m_labelDisk->setMinimumWidth(220);
    m_labelDisk->setText(QStringLiteral("Disk: 45.2 / 128 GB"));   // DEVICE-STUB (реф. поток опроса)
    v->addWidget(m_labelDisk);

    v->addSpacing(10);

    m_btnExit = new QPushButton(tr("TR_Ext(E)"), m_grpOpt);
    m_btnExit->setObjectName(QStringLiteral("btn_exit"));
    m_btnExit->setFixedWidth(212);
    v->addWidget(m_btnExit, 0, Qt::AlignHCenter);

    h5->addWidget(m_grpOpt);

    // ПРАВО — стек вью.
    m_stackView = new QStackedWidget(content);
    m_stackView->setObjectName(QStringLiteral("stackedWidget_tableview"));
    m_stackView->setFocusPolicy(Qt::NoFocus);
    h5->addWidget(m_stackView, 1);
}

void KPatientManagmentUi::InitListWidgetItem()
{
    // Реф. InitListWidgetItem: чёрный стиль + gridSize + строки-режимы (в реф. кастом-item'ы).
    m_listWidget->setStyleSheet(QStringLiteral(
        "QListWidget{background:rgb(26,26,26);border:0px;}"
        "QListWidget::item{margin-bottom:23px;color:#ddd;}"
        "QListWidget::item:selected{background:rgb(0,153,153);}"));
    m_listWidget->setGridSize(QSize(215, 103));
    m_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listWidget->addItem(new QListWidgetItem(tr("TR_Ptnt")));   // Patient
    m_listWidget->addItem(new QListWidgetItem(tr("TR_Case")));   // Case/Exam
    m_listWidget->addItem(new QListWidgetItem(tr("TR_DQueue")));  // DICOM queue
}

void KPatientManagmentUi::InitPatientlistView()
{
    // Реф. @0x7a20d8: ленивая сборка вью+тулбар, addWidget в оба стека.
    if (m_patientView)
        return;
    m_patientView = new KPatientListViewUi(this);
    m_stackView->addWidget(m_patientView);
    m_patientOpt = new KPatientListOptUi(m_stackOpt);   // реф. берёт view*; в порте свои сигналы
    m_stackOpt->addWidget(m_patientOpt);
}

void KPatientManagmentUi::InitExamlistView()
{
    if (m_examView)
        return;
    m_examView = new KExamListViewUi(this);
    m_stackView->addWidget(m_examView);
    m_examOpt = new KExamListOptUi(m_stackOpt);
    m_stackOpt->addWidget(m_examOpt);
}

void KPatientManagmentUi::InitDicomQueueView()
{
    if (m_dicomView)
        return;
    m_dicomView = new KDicomQueueViewUi(this);
    m_stackView->addWidget(m_dicomView);
    m_dicomOpt = new KDicomQueueOptUi(m_stackOpt);   // реальный тулбар (3 кнопки)
    m_stackOpt->addWidget(m_dicomOpt);
}

void KPatientManagmentUi::SwitchPage()
{
    // Реф. SwitchPage: по строке nav переключить ОБА стека (ленивая сборка при первом заходе).
    const int row = m_listWidget->currentRow();
    switch (row) {
    case E_PATIENT:
        InitPatientlistView();
        m_stackView->setCurrentWidget(m_patientView);
        m_stackOpt->setCurrentWidget(m_patientOpt);
        break;
    case E_EXAM:
        InitExamlistView();
        m_stackView->setCurrentWidget(m_examView);
        m_stackOpt->setCurrentWidget(m_examOpt);
        break;
    case E_DICOM:
        InitDicomQueueView();
        m_stackView->setCurrentWidget(m_dicomView);
        m_stackOpt->setCurrentWidget(m_dicomOpt);
        break;
    default:
        break;
    }
}

void KPatientManagmentUi::ItemClicked(const QModelIndex &)
{
    SwitchPage();
}

void KPatientManagmentUi::EntryView(E_VIEW vw)
{
    // Реф. EntryView: setCurrentRow(v) + SwitchPage.
    m_listWidget->setCurrentRow(int(vw));
    SwitchPage();
}

void KPatientManagmentUi::ExitCurrentView()
{
    close();   // реф. ExitCurrentView → закрыть оболочку
}

KPatientListViewUi *KPatientManagmentUi::PatientView() { InitPatientlistView(); return m_patientView; }
KExamListViewUi *KPatientManagmentUi::ExamView() { InitExamlistView(); return m_examView; }
KDicomQueueViewUi *KPatientManagmentUi::DicomView() { InitDicomQueueView(); return m_dicomView; }
