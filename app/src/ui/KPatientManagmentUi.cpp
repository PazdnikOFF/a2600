#include "KPatientManagmentUi.h"
#include "KPatientListWidget.h"
#include "KPatientListWidgetItem.h"
#include "sys/KSystem.h"
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
#include <QDir>

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
    // Реф. @0x7a5e58: стиль (строка @0x8a1dc0, 250 символов — у выделения и hover фон
    // ПРОЗРАЧНЫЙ, подсветку даёт сам item-виджет сменой пиксмапа), gridSize 215×103,
    // обе полосы прокрутки off, затем три строки-виджета KPatientListWidgetItem.
    m_listWidget->setStyleSheet(QStringLiteral(
        "QListWidget{ outline:0px; border: none;}"
        "QListWidget{ background-color:rgb(26, 26, 26);}"
        "QListWidget::item{ margin-bottom:23px; margin: 0px 0px 0px 0px;}"
        "QListWidget::Item:hover{background:transparent;}"
        "QListWidget::item:selected{background:transparent;}"));
    m_listWidget->setGridSize(QSize(215, 103));
    m_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Реф.: иконки лежат в ProjectPresetPath() + "patient/mainicon/" (@0x8a1ec0).
    const QString iconDir = QDir(KSystem::ProjectPresetPath()).absoluteFilePath("patient/mainicon");
    auto icons = [&iconDir](const QString &stem) {
        QMap<QString, QString> m;
        m.insert(QStringLiteral("selectIcon"),  iconDir + "/" + stem + "_select.png");
        m.insert(QStringLiteral("normalIcon"),  iconDir + "/" + stem + "_normal.png");
        m.insert(QStringLiteral("hoverIcon"),   iconDir + "/" + stem + "_hover.png");
        m.insert(QStringLiteral("disableIcon"), iconDir + "/" + stem + "_disable.png");
        return m;
    };
    // Реф. тройка: (patientlist, TR_PInfo, "(F3)"), (examlist, TR_PList, "(Alt+F3)"),
    // (report, TR_DQueue, "(F10)"). Раньше в порте стояли TR_Ptnt/TR_Case и не было суффиксов.
    struct Row { const char *stem; const char *key; const char *suffix; };
    static const Row rows[] = {
        {"patientlist", "TR_PInfo",  "(F3)"},
        {"examlist",    "TR_PList",  "(Alt+F3)"},
        {"report",      "TR_DQueue", "(F10)"},
    };
    for (const Row &r : rows) {
        const QString label = tr(r.key) + QString::fromLatin1(r.suffix);
        auto *w = new KPatientListWidgetItem(icons(QString::fromLatin1(r.stem)), label);
        w->SetFontSize(15);                                   // реф. SetFontSize(15)
        auto *li = new QListWidgetItem;
        li->setData(Qt::SizeHintRole, QSize(211, 80));        // реф. QSize(0xd3, 0x50)
        setElidedFrame(w, 6, label, QString::fromLatin1(r.suffix));
        m_listWidget->insertItem(m_listWidget->count(), li);
        m_listWidget->setItemWidget(li, w);
        m_navItems.append(w);
    }
    // Порт: список без прокрутки должен вмещать все строки целиком — иначе подпись
    // последней строки обрезается вьюпортом (поймано скриншотом). Высота = число строк ×
    // шаг сетки 103. В реф. высоту задаёт Designer-раскладка grp_opt.
    m_listWidget->setFixedHeight(m_listWidget->count() * 103);

    // Реф. хвост @0x7a6a6c: первая строка — выбранная.
    if (!m_navItems.isEmpty())
        m_navItems.first()->Select();
}

void KPatientManagmentUi::RefreshNavSelection(int row)
{
    // Реф.: подсветка навигации = смена пиксмапа у строк (Select/UnSelect), а НЕ стиль списка.
    for (int i = 0; i < m_navItems.size(); ++i) {
        if (i == row)
            m_navItems[i]->Select();
        else
            m_navItems[i]->UnSelect();
    }
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

void KPatientManagmentUi::ItemClicked(const QModelIndex &idx)
{
    RefreshNavSelection(idx.row());   // реф.: подсветка = пиксмап строки, не стиль списка
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
