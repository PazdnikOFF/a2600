#include "KPatientListSearch.h"

#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegExpValidator>
#include <QVBoxLayout>

namespace {
QLabel *mkLbl(QWidget *p, const QString &t, const char *name)
{
    QLabel *l = new QLabel(t, p);
    l->setObjectName(QString::fromLatin1(name));
    return l;
}
// Ячейка «подпись сверху / поле снизу».
QWidget *cell(QWidget *p, const QString &cap, const char *capName, QWidget *field)
{
    QWidget *w = new QWidget(p);
    QVBoxLayout *v = new QVBoxLayout(w);
    v->setContentsMargins(0, 0, 0, 0);
    v->addWidget(mkLbl(w, cap, capName));
    v->addWidget(field);
    return w;
}
} // namespace

KPatientListSearch::KPatientListSearch(QWidget *parent)
    : QWidget(parent)
{
    // Реф. ctor @0x7d5cf0: QWidget(parent,0) → setupUi → InitWidget → InitConnect →
    // InitWidgetFileter. Без KDialog/SetKStyle/модальности.
    setupUi();
}

void KPatientListSearch::setupUi()
{
    setObjectName(QStringLiteral("KPatientListSearch"));
    setMinimumHeight(170);   // реф. фикс. высота строки

    QHBoxLayout *root = new QHBoxLayout(this);

    QGridLayout *g = new QGridLayout();
    g->setHorizontalSpacing(30);

    auto mkEdit = [&](const char *name) {
        QLineEdit *e = new QLineEdit(this);
        e->setObjectName(QString::fromLatin1(name));
        m_edits << e;
        return e;
    };
    auto ageValidator = [&](QLineEdit *e) {
        e->setValidator(new QRegExpValidator(
            QRegExp(QStringLiteral("^([1-9]|[1-9]\\d|(1[0-9][0-9]))$")), e));   // 1..199
    };

    // --- Ряд 1 ---
    g->addWidget(cell(this, tr("TR_PID2"), "label_patientid", mkEdit("edit_patientid")), 0, 0);
    g->addWidget(cell(this, tr("TR_Nme:"), "label_name", mkEdit("edit_name")), 0, 1);

    // Пол + возраст (диапазон) — составная ячейка.
    QWidget *wGA = new QWidget(this);
    QVBoxLayout *vGA = new QVBoxLayout(wGA); vGA->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout *hGAtop = new QHBoxLayout();
    hGAtop->addWidget(mkLbl(wGA, tr("TR_Gdr:"), "label_gdr"));
    hGAtop->addWidget(mkLbl(wGA, tr("TR_Age:"), "label_age"), 1);
    vGA->addLayout(hGAtop);
    QHBoxLayout *hGAbot = new QHBoxLayout();
    QComboBox *cmbGdr = new QComboBox(wGA);
    cmbGdr->setObjectName(QStringLiteral("cmb_gdr"));
    cmbGdr->addItem(QString(), 3);        // реф. s_vct_gender: пусто(3)/М(1)/Ж(0)/неизв(2)
    cmbGdr->addItem(tr("TR_M"), 1);
    cmbGdr->addItem(tr("TR_F"), 0);
    cmbGdr->addItem(tr("TR_Nknwn"), 2);
    m_combos << cmbGdr;
    hGAbot->addWidget(cmbGdr);
    QLineEdit *ageS = mkEdit("edit_start_age"); ageS->setFixedWidth(52);
    ageS->setInputMethodHints(Qt::ImhDigitsOnly); ageValidator(ageS);
    QLineEdit *ageE = mkEdit("edit_end_age"); ageE->setFixedWidth(52); ageValidator(ageE);
    hGAbot->addWidget(ageS);
    hGAbot->addWidget(mkLbl(wGA, QStringLiteral("--"), "label_age_spar"));
    hGAbot->addWidget(ageE);
    vGA->addLayout(hGAbot);
    g->addWidget(wGA, 0, 2);

    // Пункт обследования (device-список).
    QComboBox *cmbItem = new QComboBox(this);
    cmbItem->setObjectName(QStringLiteral("cmb_examitem"));
    cmbItem->addItem(QString());          // реф. "" (0x7ffffffe)
    cmbItem->addItem(tr("TR_All"));        // реф. TR_All (0x7fffffff) + GetEndoClassToQstring (device)
    m_combos << cmbItem;
    g->addWidget(cell(this, tr("TR_EItem:"), "label_examitem", cmbItem), 0, 3);

    // --- Ряд 2 ---
    g->addWidget(cell(this, tr("TR_Aplct:"), "label_applicant", mkEdit("edit_applicant")), 1, 0);

    QComboBox *cmbStatus = new QComboBox(this);
    cmbStatus->setObjectName(QStringLiteral("cmb_status"));
    cmbStatus->addItem(QString());        // реф. "" (0x80000000)
    cmbStatus->addItem(tr("TR_All"));
    cmbStatus->addItem(tr("TR_NExamined"));
    cmbStatus->addItem(tr("TR_Exmed"));
    m_combos << cmbStatus;
    g->addWidget(cell(this, tr("TR_Stts:"), "label_status", cmbStatus), 1, 1);

    // Дата-от (+ календарь).
    QWidget *wDS = new QWidget(this);
    QVBoxLayout *vDS = new QVBoxLayout(wDS); vDS->setContentsMargins(0, 0, 0, 0);
    vDS->addWidget(mkLbl(wDS, tr("TR_ADate:"), "label_date_start"));
    QHBoxLayout *hDS = new QHBoxLayout();
    QDateEdit *dS = new QDateEdit(wDS);    // реф. KPatientDateEdit → QDateEdit
    dS->setObjectName(QStringLiteral("dateEdit_start"));
    dS->setCalendarPopup(true); dS->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    dS->setDate(QDate::currentDate()); m_dates << dS;
    QPushButton *bS = new QPushButton(wDS); bS->setObjectName(QStringLiteral("btnStartDate"));
    bS->setFixedSize(31, 26);
    connect(bS, &QPushButton::clicked, dS, [dS]() { dS->setFocus(); });
    hDS->addWidget(dS); hDS->addWidget(bS);
    vDS->addLayout(hDS);
    g->addWidget(wDS, 1, 2);

    // Дата-до (без подписи; «--» отделяет от даты-от).
    QWidget *wDE = new QWidget(this);
    QVBoxLayout *vDE = new QVBoxLayout(wDE); vDE->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout *hDEtop = new QHBoxLayout();
    hDEtop->addWidget(mkLbl(wDE, QStringLiteral("--"), "label_spar"));
    hDEtop->addStretch(1);
    vDE->addLayout(hDEtop);
    QHBoxLayout *hDE = new QHBoxLayout();
    QDateEdit *dE = new QDateEdit(wDE);
    dE->setObjectName(QStringLiteral("dateEdit_end"));
    dE->setCalendarPopup(true); dE->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    dE->setDate(QDate::currentDate()); m_dates << dE;
    QPushButton *bE = new QPushButton(wDE); bE->setObjectName(QStringLiteral("btnEndDate"));
    bE->setFixedSize(31, 26);
    connect(bE, &QPushButton::clicked, dE, [dE]() { dE->setFocus(); });
    hDE->addWidget(dE); hDE->addWidget(bE);
    vDE->addLayout(hDE);
    g->addWidget(wDE, 1, 3);

    root->addLayout(g, 1);

    // --- Колонка кнопок Search/Reset ---
    QVBoxLayout *vBtn = new QVBoxLayout();
    QPushButton *btnSearch = new QPushButton(tr("TR_Sch(F11)"), this);
    btnSearch->setObjectName(QStringLiteral("btn_search"));
    btnSearch->setFixedWidth(201);
    // btn_search → SlotToStartSearch → emit QueryItems — запрос в БД downstream (device), no-op.
    QPushButton *btnReset = new QPushButton(tr("TR_Rst2(F12)"), this);
    btnReset->setObjectName(QStringLiteral("btn_reset"));
    btnReset->setFixedWidth(201);
    connect(btnReset, &QPushButton::clicked, this, &KPatientListSearch::resetSearchData);
    vBtn->addWidget(btnSearch);
    vBtn->addWidget(btnReset);
    vBtn->addStretch(1);
    root->addLayout(vBtn);
}

void KPatientListSearch::resetSearchData()
{
    // Реф. SlotToResetSearchData: очистить поля, сбросить комбо на индекс 0, даты на сегодня.
    for (QLineEdit *e : m_edits)
        e->clear();
    for (QComboBox *c : m_combos)
        c->setCurrentIndex(0);
    for (QDateEdit *d : m_dates)
        d->setDate(QDate::currentDate());
}
