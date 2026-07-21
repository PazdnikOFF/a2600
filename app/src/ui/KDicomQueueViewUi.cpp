#include "KDicomQueueViewUi.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

KDicomQueueViewUi::KDicomQueueViewUi(QWidget *parent)
    : QWidget(parent)
{
    // Реф. ctor @0x8102f8: QWidget(parent,0) + KObject-mixin → setupUi → new KDicomQueueSearch →
    // InitTable → InitPageButton → InitLineEditPage → SubscribeMsg (device) → Initconnect.
    setupUi();
    resize(1630, 1034);   // реф. resize
}

void KDicomQueueViewUi::setupUi()
{
    setObjectName(QStringLiteral("KDicomQueueViewUi"));

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setObjectName(QStringLiteral("verticalLayout_2"));
    root->setContentsMargins(0, 0, 0, 0);

    QGroupBox *grp = new QGroupBox(this);
    grp->setObjectName(QStringLiteral("grp_view"));
    grp->setTitle(QString());   // реф. пустой заголовок
    QVBoxLayout *v = new QVBoxLayout(grp);
    v->setObjectName(QStringLiteral("verticalLayout"));
    v->setContentsMargins(18, 4, 18, 4);

    // Хост панели поиска (реф. KDicomQueueSearch) → плейсхолдер-фильтр.
    QWidget *wSearch = new QWidget(grp);
    wSearch->setObjectName(QStringLiteral("widget_search"));
    QHBoxLayout *hs = new QHBoxLayout(wSearch);
    hs->setContentsMargins(0, 0, 0, 0);
    QLineEdit *filter = new QLineEdit(wSearch);
    filter->setPlaceholderText(tr("TR_Sch(F11)"));
    hs->addWidget(filter);
    hs->addStretch(1);
    v->addWidget(wSearch);

    // Таблица очереди (реф. KTableView, 11 колонок; col10 скрыт; col0 — чекбокс).
    QTableWidget *table = new QTableWidget(grp);
    table->setObjectName(QStringLiteral("tableView"));
    table->setColumnCount(11);
    table->setHorizontalHeaderLabels({QString(), tr("TR_Tpe2"), tr("TR_Svce"), tr("TR_PID"),
                                      tr("TR_Nme"), tr("TR_STime"), tr("TR_Fname"), tr("TR_Sze"),
                                      tr("TR_Stts"), tr("TR_Dtls1"), QStringLiteral("id")});
    table->hideColumn(10);   // реф. DicomCommandUID скрыт
    table->verticalHeader()->hide();
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionsClickable(true);   // реф. сортировка по клику
    table->setColumnWidth(0, 40);   // col0 — колонка выбора
    v->addWidget(table, 1);   // строки — device (БД очереди)

    // Пейджер-бар (реф. widget — абсолютная геометрия).
    QWidget *pager = new QWidget(grp);
    pager->setObjectName(QStringLiteral("widget"));
    pager->setMinimumHeight(70);
    QLabel *lblRec = new QLabel(tr("TR_Rcds"), pager);   // реф. label_db_record "TextLabel"
    lblRec->setObjectName(QStringLiteral("label_db_record"));
    lblRec->setGeometry(20, 1, 158, 65);
    QLabel *lblHint = new QLabel(pager);                  // реф. label_hint "TextLabel"
    lblHint->setObjectName(QStringLiteral("label_hint"));
    lblHint->setGeometry(182, 1, 585, 65);
    auto pageBtn = [&](const char *name, const QString &glyph, int x) {
        QPushButton *b = new QPushButton(glyph, pager);   // реф. KPagePushButton → QPushButton
        b->setObjectName(QString::fromLatin1(name));
        b->setGeometry(x, 10, 52, 34);
        return b;
    };
    pageBtn("btn_home", QStringLiteral("|<"), 767);   // реф. на первую
    pageBtn("btn_pre", QStringLiteral("<"), 835);     // реф. пред.
    QLineEdit *edPage = new QLineEdit(pager);          // реф. KPageLineEdit → QLineEdit
    edPage->setObjectName(QStringLiteral("edit_page"));
    edPage->setGeometry(911, 10, 52, 32);
    edPage->setValidator(new QIntValidator(1, 99999, edPage));
    QLabel *lblPage = new QLabel(QStringLiteral("1"), pager);   // реф. label_page "1"
    lblPage->setObjectName(QStringLiteral("label_page"));
    lblPage->setGeometry(980, 14, 41, 26);
    lblPage->setAlignment(Qt::AlignCenter);
    pageBtn("btn_next", QStringLiteral(">"), 1021);   // реф. след.
    pageBtn("btn_tail", QStringLiteral(">|"), 1090);  // реф. на последнюю
    v->addWidget(pager);
    // Пейджинг/выделение/сортировка — чистый UI; OnGetDicomQueueDataFromDB/OnQuery/
    // OnResendSelectedData/SubscribeMsg — DEVICE, не подключаем.

    root->addWidget(grp);
}
