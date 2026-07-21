#include "KFunTest.h"

#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QStringListModel>
#include <QVBoxLayout>
#include <QWidget>

KFunTest::KFunTest(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x836c30: setupUi → SetKStyle(5) → title "Function Test" → InitWidget → connect.
    setupUi();
    SetKStyle(KDLG_W640);                 // реф. SetKStyle(5)
    SetTitle(tr("Function Test"));         // реф. setWindowTitle (литерал)
    initWidget();

    connect(btn_start, &QPushButton::clicked, this, &KFunTest::ClickStart);
    connect(btn_presure, &QPushButton::clicked, this, &KFunTest::ClickPresureTest);
    connect(btn_totest, &QPushButton::clicked, this, &KFunTest::ClickToTest);
    connect(btn_tolibrary, &QPushButton::clicked, this, &KFunTest::ClickToCaseLibrary);
    connect(btn_import, &QPushButton::clicked, this, &KFunTest::OpenImportRules);
}

void KFunTest::setupUi()
{
    // Реф. Ui_KFunTest::setupUi @0x8379a8. Абсолютная геометрия внутри 640×480. Кладём в
    // ContentArea() (под титул-баром KDialog). setStyleSheet НЕТ. Списки — QListView (реф.
    // QListViewTmp — подкласс QListView).
    setObjectName(QStringLiteral("KFunTest"));
    resize(640, 515);   // реф. 640×480; +35 под титул KDialog

    QWidget *host = ContentArea();

    // --- groupBox "Use Case Library" (20,50,276,331) ---
    QGroupBox *groupBox = new QGroupBox(host);
    groupBox->setObjectName(QStringLiteral("groupBox"));
    groupBox->setTitle(tr("Use Case Library"));
    groupBox->setGeometry(20, 50, 276, 331);
    QGridLayout *g1 = new QGridLayout(groupBox);
    g1->setObjectName(QStringLiteral("gridLayout"));
    table_caselibrary = new QListView(groupBox);
    table_caselibrary->setObjectName(QStringLiteral("table_caselibrary"));
    g1->addWidget(table_caselibrary, 0, 0);

    // --- groupBox_2 "Execute Test Cases" (350,50,276,331) ---
    QGroupBox *groupBox_2 = new QGroupBox(host);
    groupBox_2->setObjectName(QStringLiteral("groupBox_2"));
    groupBox_2->setTitle(tr("Execute Test Cases"));
    groupBox_2->setGeometry(350, 50, 276, 331);
    QGridLayout *g2 = new QGridLayout(groupBox_2);
    g2->setObjectName(QStringLiteral("gridLayout_2"));
    table_testcase = new QListView(groupBox_2);
    table_testcase->setObjectName(QStringLiteral("table_testcase"));
    g2->addWidget(table_testcase, 0, 0);

    // --- splitter (300,180,40,90) вертикальный: >> / << ---
    QSplitter *splitter = new QSplitter(host);
    splitter->setObjectName(QStringLiteral("splitter"));
    splitter->setOrientation(Qt::Vertical);
    splitter->setGeometry(300, 180, 40, 90);
    btn_totest = new QPushButton(splitter);
    btn_totest->setObjectName(QStringLiteral("btn_totest"));
    btn_totest->setText(QStringLiteral(">>"));
    btn_totest->setFixedSize(40, 40);
    splitter->addWidget(btn_totest);
    btn_tolibrary = new QPushButton(splitter);
    btn_tolibrary->setObjectName(QStringLiteral("btn_tolibrary"));
    btn_tolibrary->setText(QStringLiteral("<<"));
    btn_tolibrary->setFixedSize(40, 40);
    splitter->addWidget(btn_tolibrary);

    // --- btn_start / btn_presure ---
    btn_start = new QPushButton(host);
    btn_start->setObjectName(QStringLiteral("btn_start"));
    btn_start->setText(tr("Start"));
    btn_start->setGeometry(269, 420, 110, 26);
    btn_presure = new QPushButton(host);
    btn_presure->setObjectName(QStringLiteral("btn_presure"));
    btn_presure->setText(tr("Presure Test"));
    btn_presure->setGeometry(250, 450, 141, 26);

    // --- layoutWidget (40,390,171,61): Count / Speed ---
    QWidget *layoutWidget = new QWidget(host);
    layoutWidget->setObjectName(QStringLiteral("layoutWidget"));
    layoutWidget->setGeometry(40, 390, 171, 61);
    QGridLayout *g3 = new QGridLayout(layoutWidget);
    g3->setObjectName(QStringLiteral("gridLayout_3"));
    g3->setContentsMargins(0, 0, 0, 0);
    QLabel *label = new QLabel(layoutWidget);
    label->setObjectName(QStringLiteral("label"));
    label->setText(tr("Count:"));
    g3->addWidget(label, 0, 0);
    spinBox_count = new QSpinBox(layoutWidget);
    spinBox_count->setObjectName(QStringLiteral("spinBox_count"));
    g3->addWidget(spinBox_count, 0, 1);
    QLabel *label_2 = new QLabel(layoutWidget);
    label_2->setObjectName(QStringLiteral("label_2"));
    label_2->setText(tr("Speed:"));
    g3->addWidget(label_2, 1, 0);
    cmb_speed = new QComboBox(layoutWidget);
    cmb_speed->setObjectName(QStringLiteral("cmb_speed"));
    g3->addWidget(cmb_speed, 1, 1);

    // --- widget (440,400,181,61): checklog + ImportCheckRules ---
    QWidget *widget = new QWidget(host);
    widget->setObjectName(QStringLiteral("widget"));
    widget->setGeometry(440, 400, 181, 61);
    QVBoxLayout *vl = new QVBoxLayout(widget);
    vl->setObjectName(QStringLiteral("verticalLayout"));
    vl->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout *hl = new QHBoxLayout();
    hl->setObjectName(QStringLiteral("horizontalLayout"));
    QLabel *label_3 = new QLabel(widget);
    label_3->setObjectName(QStringLiteral("label_3"));
    label_3->setText(tr("checklog"));
    hl->addWidget(label_3);
    comboBox_cheklogstatus = new QComboBox(widget);
    comboBox_cheklogstatus->setObjectName(QStringLiteral("comboBox_cheklogstatus"));
    comboBox_cheklogstatus->addItem(tr("OPEN"));
    comboBox_cheklogstatus->addItem(tr("CLOSE"));
    hl->addWidget(comboBox_cheklogstatus);
    vl->addLayout(hl);
    btn_import = new QPushButton(widget);
    btn_import->setObjectName(QStringLiteral("btn_import"));
    btn_import->setText(tr("ImportCheckRules"));
    vl->addWidget(btn_import);
}

void KFunTest::initWidget()
{
    // Реф. @0x835fc8 (layout-часть): скорость 1/2/0.5, presure скрыт, списки — NoEdit/MultiSel/
    // NoFocus + QStringListModel (наполнение из /tmp/testcaselist.txt и autotest/casefile —
    // DEVICE, в превью пусто).
    cmb_speed->addItem(QStringLiteral("1"));
    cmb_speed->addItem(QStringLiteral("2"));
    cmb_speed->addItem(QStringLiteral("0.5"));
    btn_presure->setVisible(false);

    for (QListView *lv : {table_caselibrary, table_testcase}) {
        lv->setEditTriggers(QAbstractItemView::NoEditTriggers);
        lv->setSelectionBehavior(QAbstractItemView::SelectRows);
        lv->setSelectionMode(QAbstractItemView::MultiSelection);
        lv->setFocusPolicy(Qt::NoFocus);
        lv->setModel(new QStringListModel(lv));
    }
}

void KFunTest::ClickStart() { /* реф.: запуск тест-кейсов (движок автотеста) — device */ }
void KFunTest::ClickPresureTest() { /* реф.: стресс-тест — device */ }

void KFunTest::ClickToTest()
{
    // Реф.: перенос выделенных кейсов из библиотеки в исполнение (модели) — логика device-данных.
}

void KFunTest::ClickToCaseLibrary()
{
    // Реф.: обратный перенос — device-данные.
}

void KFunTest::OpenImportRules()
{
    // Реф.: открывает диалог KImportRules (уже портирован). В превью — заглушка, чтобы не
    // блокировать модальным exec при скриншоте.
}
