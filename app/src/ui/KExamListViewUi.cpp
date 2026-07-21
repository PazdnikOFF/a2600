#include "KExamListViewUi.h"

#include <QGroupBox>
#include <QHeaderView>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

KExamListViewUi::KExamListViewUi(QWidget *parent)
    : QWidget(parent)
{
    // Реф. ctor @0x801468: QWidget(parent,0) + KObject(0x7d8) → setupUi → InitTable →
    // InitPageButton → InitLineEditPage → new KExamListSearch → SubscribeMsg×8 → InitConnect.
    setupUi();
    resize(1630, 1034);   // реф. resize
}

void KExamListViewUi::setupUi()
{
    setObjectName(QStringLiteral("KExamListViewUi"));

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setObjectName(QStringLiteral("verticalLayout_2"));
    root->setContentsMargins(0, 0, 0, 0);

    QGroupBox *grp = new QGroupBox(this);
    grp->setObjectName(QStringLiteral("grp_view"));
    grp->setTitle(QString());
    QVBoxLayout *v = new QVBoxLayout(grp);
    v->setObjectName(QStringLiteral("verticalLayout"));
    v->setContentsMargins(18, 4, 18, 4);

    // Хост панели поиска (реф. KExamListSearch) → плейсхолдер-фильтр.
    QWidget *wSearch = new QWidget(grp);
    wSearch->setObjectName(QStringLiteral("widget_search"));
    QVBoxLayout *vs = new QVBoxLayout(wSearch); vs->setContentsMargins(0, 0, 0, 0);
    QLineEdit *filter = new QLineEdit(wSearch);
    filter->setPlaceholderText(tr("TR_Sch(F11)"));
    vs->addWidget(filter);
    v->addWidget(wSearch);

    // Таблица списка обследований (реф. KTableView, 20 колонок; col0 чекбокс).
    QTableWidget *table = new QTableWidget(grp);
    table->setObjectName(QStringLiteral("tableView"));
    table->setColumnCount(20);
    table->setHorizontalHeaderLabels({QString(), tr("TR_PID"), tr("TR_Nme"), tr("TR_Gdr"),
        tr("TR_Age"), tr("TR_EmDate"), tr("TR_Aplct"), tr("TR_Dctr"), tr("TR_EModel"),
        tr("TR_ESN"), tr("TR_Stts"), tr("TR_ENo"), tr("TR_Pctr"), tr("TR_Vdeo2"), tr("TR_DoB"),
        tr("TR_Tel"), tr("TR_BNo"), tr("TR_ANumber"), tr("TR_CField1"), tr("TR_CField2")});
    table->verticalHeader()->hide();
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->horizontalHeader()->setSectionsClickable(true);   // реф. сортировка
    table->setColumnWidth(0, 40);
    v->addWidget(table, 1);   // строки — device (БД обследований)

    // Пейджер-бар (реф. widget — абсолютная геометрия).
    QWidget *pager = new QWidget(grp);
    pager->setObjectName(QStringLiteral("widget"));
    pager->setMinimumHeight(70);
    QLabel *lblRec = new QLabel(tr("TR_Rcds"), pager);   // реф. label_db_record
    lblRec->setObjectName(QStringLiteral("label_db_record"));
    lblRec->setGeometry(20, 1, 177, 65);
    QLabel *lblHint = new QLabel(pager);                 // реф. label_hint
    lblHint->setObjectName(QStringLiteral("label_hint"));
    lblHint->setGeometry(182, 1, 766, 65);
    auto pageBtn = [&](const char *name, const QString &glyph, int x) {
        QPushButton *b = new QPushButton(glyph, pager);   // реф. KPagePushButton → QPushButton
        b->setObjectName(QString::fromLatin1(name));
        b->setGeometry(x, 10, 52, 52);
        return b;
    };
    pageBtn("btn_home", QStringLiteral("|<"), 767);
    pageBtn("btn_pre", QStringLiteral("<"), 835);
    QLineEdit *edPage = new QLineEdit(pager);   // реф. KPageLineEdit → QLineEdit
    edPage->setObjectName(QStringLiteral("edit_page"));
    edPage->setGeometry(911, 10, 52, 52);
    edPage->setValidator(new QIntValidator(1, 99999, edPage));
    QLabel *lblPage = new QLabel(QStringLiteral("1"), pager);   // реф. label_page
    lblPage->setObjectName(QStringLiteral("label_page"));
    lblPage->setGeometry(980, 14, 41, 44);
    lblPage->setAlignment(Qt::AlignCenter);
    pageBtn("btn_next", QStringLiteral(">"), 1021);
    pageBtn("btn_tail", QStringLiteral(">|"), 1090);
    v->addWidget(pager);
    // Пагинация/выделение/сортировка — UI; OnGetExamListDataFromDB/OnQuery/SubscribeMsg — DEVICE.

    root->addWidget(grp);
}
