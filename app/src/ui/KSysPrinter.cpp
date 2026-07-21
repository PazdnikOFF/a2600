#include "KSysPrinter.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

KSysPrinter::KSysPrinter(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. Initialize @0x79c0e0: setupUi → frameless → тема-QSS → setGeometry 1680×900 →
    // конфиг таблицы → RefreshTable (device) → RegisterSignalConnect. У нас — контент в
    // ContentArea, крупный диалог; SetKStyle не вызывается в реф., ставим FULLSCREEN под 1680×900.
    setupUi();
    SetKStyle(KDLG_FULLSCREEN);         // реф. setGeometry на весь экран (SetKStyle нет)
    SetTitle(tr("TR_SPrinter"));        // титул-бар (реф. титул-лейбл тот же TR_SPrinter)
}

void KSysPrinter::setupUi()
{
    setObjectName(QStringLiteral("KSysPrinter"));

    QWidget *host = ContentArea();
    QVBoxLayout *root = new QVBoxLayout(host);
    root->setObjectName(QStringLiteral("verticalLayout"));

    // Реф. m_pFrame (StyledPanel) с заголовком и таблицей.
    QFrame *frame = new QFrame(host);
    frame->setObjectName(QStringLiteral("m_pFrame"));
    frame->setFrameShape(QFrame::StyledPanel);
    QVBoxLayout *vf = new QVBoxLayout(frame);

    // Реф. m_pTitleLabel TR_SPrinter — центр, крупный.
    QLabel *title = new QLabel(frame);
    title->setObjectName(QStringLiteral("m_pTitleLabel"));
    title->setText(tr("TR_SPrinter"));
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(QStringLiteral("font-weight:bold; font-size:22px;"));
    vf->addWidget(title);

    // Реф. m_pTableWidget — 5 колонок, строки device-populated (пусто).
    QTableWidget *table = new QTableWidget(frame);
    table->setObjectName(QStringLiteral("m_pTableWidget"));
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({tr("TR_PName"), tr("TR_SType"),
                                      tr("TR_CType2"), tr("TR_Dvc"), tr("TR_Dflt1")});
    table->verticalHeader()->hide();                              // реф. verticalHeader()->hide()
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);    // реф. NoEditTriggers
    table->setSelectionBehavior(QAbstractItemView::SelectRows);   // реф. SelectRows
    table->setSelectionMode(QAbstractItemView::SingleSelection);  // реф. SingleSelection
    table->setAlternatingRowColors(true);                         // реф. setAlternatingRowColors
    table->horizontalHeader()->setStretchLastSection(true);       // реф. последняя секция Stretch
    table->horizontalHeader()->setSectionsMovable(true);
    table->setStyleSheet(QStringLiteral("background-color:rgb(20, 21, 25);"));  // реф. литерал
    for (int c = 0; c < 4; ++c)
        table->setColumnWidth(c, 200);
    vf->addWidget(table, 1);

    root->addWidget(frame, 1);

    // Реф. ряд кнопок (layoutWidget/m_pHorizontalLayout) с растяжками между.
    QHBoxLayout *hb = new QHBoxLayout();
    hb->setObjectName(QStringLiteral("m_pHorizontalLayout"));
    hb->setContentsMargins(0, 10, 0, 10);
    auto mkBtn = [&](const char *name, const QString &text) {
        QPushButton *b = new QPushButton(host);
        b->setObjectName(QString::fromLatin1(name));
        b->setText(text);
        b->setMinimumWidth(140);
        return b;
    };
    QPushButton *btnAdd = mkBtn("m_pAddPrinterBtn", tr("TR_APrinter"));
    QPushButton *btnDel = mkBtn("m_pDelPrinterBtn", tr("TR_DPrinter2"));
    QPushButton *btnDef = mkBtn("m_pDefaultPrinterBtn", tr("TR_DPrinter"));
    QPushButton *btnSet = mkBtn("m_pPrintSettingsBtn", tr("TR_SPrinter"));
    hb->addStretch(1);
    hb->addWidget(btnAdd);
    hb->addStretch(1);
    hb->addWidget(btnDel);
    hb->addStretch(1);
    hb->addWidget(btnDef);
    hb->addStretch(1);
    hb->addWidget(btnSet);
    hb->addStretch(1);
    root->addLayout(hb);
    // Add/Del/Default/PrintSettings → KPrinterManager/CUPS — DEVICE, не подключаем.

    // Реф. OnTableWidgetSelectionChanged: Del/Default/PrintSettings активны только при
    // выделении строки (таблица пуста → выключены).
    btnDel->setEnabled(false);
    btnDef->setEnabled(false);
    btnSet->setEnabled(false);
    connect(table, &QTableWidget::itemSelectionChanged, this, [=]() {
        bool has = !table->selectedItems().isEmpty();
        btnDel->setEnabled(has);
        btnDef->setEnabled(has);
        btnSet->setEnabled(has);
    });

    // Реф. m_pExit — абсолютно снизу-справа; у нас — отдельный ряд, прижат вправо.
    QHBoxLayout *hexit = new QHBoxLayout();
    hexit->addStretch(1);
    QPushButton *btnExit = new QPushButton(host);
    btnExit->setObjectName(QStringLiteral("m_pExit"));
    btnExit->setText(tr("TR_Ext"));
    btnExit->setFixedWidth(175);   // реф. 175×31
    connect(btnExit, &QPushButton::clicked, this, &QWidget::close);   // реф. clicked()→close
    hexit->addWidget(btnExit);
    root->addLayout(hexit);
}
