#include "KDicomQueueSearch.h"
#include "KPatientDateEdit.h"

#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStringList>
#include <QDate>

static const QString kDateFmt = QStringLiteral("yyyy-MM-dd");   // реф. KSystemSet::GetDateFormatString (device)

KDicomQueueSearch::KDicomQueueSearch(QWidget *parent)
    : QWidget(parent)
{
    // Реф. ctor @0x808f40: setupUi → InitWidget → InitConnect → InitWidgetFileter.
    setupUi();
}

void KDicomQueueSearch::setupUi()
{
    setObjectName(QStringLiteral("KDicomQueueSearch"));

    auto mkLbl = [&](QWidget *p, const QString &t, const char *name) {
        QLabel *l = new QLabel(t, p);
        l->setObjectName(QString::fromLatin1(name));
        return l;
    };
    auto cell = [&](const QString &cap, const char *capName, QWidget *field) {
        QWidget *w = new QWidget(this);
        QVBoxLayout *v = new QVBoxLayout(w);
        v->setContentsMargins(0, 0, 0, 0);
        v->addWidget(mkLbl(w, cap, capName));
        v->addWidget(field);
        return w;
    };
    auto mkEdit = [&](const char *name) {
        QLineEdit *e = new QLineEdit(this);
        e->setObjectName(QString::fromLatin1(name));
        return e;
    };

    QGridLayout *g = new QGridLayout(this);

    m_editPatientId = mkEdit("edit_patientid");
    g->addWidget(cell(tr("TR_PID2"), "label_patientid", m_editPatientId), 0, 0);
    m_editName = mkEdit("edit_name");
    g->addWidget(cell(tr("TR_Nme:"), "label_name", m_editName), 0, 1);

    // DICOM-тип сервиса (CommandType).
    m_cmbMsgType = new QComboBox(this);
    m_cmbMsgType->setObjectName(QStringLiteral("cmb_msgtype"));
    m_cmbMsgType->addItem(QString());   // idx0 пусто
    for (const char *k : {"TR_All", "TR_MPPS", "TR_Strge", "TR_SCommitment"})
        m_cmbMsgType->addItem(tr(k));
    m_cmbMsgType->setCurrentIndex(0);
    g->addWidget(cell(tr("TR_Tpe2:"), "label_msgtype", m_cmbMsgType), 0, 2);

    // Статус отправки (CommandStatus).
    m_cmbStatus = new QComboBox(this);
    m_cmbStatus->setObjectName(QStringLiteral("cmb_status"));
    m_cmbStatus->addItem(QString());
    for (const char *k : {"TR_All", "TR_Sccssfl", "TR_Fl1", "TR_Upldng1", "TR_Wtng"})
        m_cmbStatus->addItem(tr(k));
    m_cmbStatus->setCurrentIndex(0);
    g->addWidget(cell(tr("TR_Stts:"), "label_status", m_cmbStatus), 0, 3);

    // Диапазон дат (LastUpdateDate) — 2 KPatientDateEdit + кнопки календаря.
    auto mkDate = [&](const char *name, const char *btnName) {
        QWidget *w = new QWidget(this);
        QHBoxLayout *h = new QHBoxLayout(w);
        h->setContentsMargins(0, 0, 0, 0);
        KPatientDateEdit *d = new KPatientDateEdit(w);
        d->setObjectName(QString::fromLatin1(name));
        d->setDisplayFormat(kDateFmt);
        d->setDate(KPatientDateEdit::InvalidDate());
        QPushButton *b = new QPushButton(w);
        b->setObjectName(QString::fromLatin1(btnName));
        b->setFixedWidth(24);
        h->addWidget(d); h->addWidget(b);
        return qMakePair(w, d);
    };
    auto ds = mkDate("dateEdit_start", "btnStartDate");
    m_dateStart = ds.second;
    QWidget *wDS = new QWidget(this);
    QVBoxLayout *vDS = new QVBoxLayout(wDS); vDS->setContentsMargins(0, 0, 0, 0);
    vDS->addWidget(mkLbl(wDS, tr("TR_STime2"), "label_date_start"));
    vDS->addWidget(ds.first);
    g->addWidget(wDS, 1, 0, 1, 2);

    auto de = mkDate("dateEdit_end", "btnEndDate");
    m_dateEnd = de.second;
    QWidget *wDE = new QWidget(this);
    QVBoxLayout *vDE = new QVBoxLayout(wDE); vDE->setContentsMargins(0, 0, 0, 0);
    vDE->addWidget(mkLbl(wDE, QStringLiteral("--"), "label_spar"));
    vDE->addWidget(de.first);
    g->addWidget(wDE, 1, 2);

    QVBoxLayout *vBtn = new QVBoxLayout();
    m_btnSearch = new QPushButton(tr("TR_Sch") + QStringLiteral("(F11)"), this);
    m_btnSearch->setObjectName(QStringLiteral("btn_search"));
    m_btnReset = new QPushButton(tr("TR_Rst2") + QStringLiteral("(F12)"), this);
    m_btnReset->setObjectName(QStringLiteral("btn_reset"));
    vBtn->addWidget(m_btnSearch);
    vBtn->addWidget(m_btnReset);
    g->addLayout(vBtn, 1, 3);

    setTabOrder(m_editPatientId, m_editName);
    setTabOrder(m_editName, m_cmbMsgType);
    setTabOrder(m_cmbMsgType, m_cmbStatus);
    setTabOrder(m_cmbStatus, m_dateStart);
    setTabOrder(m_dateStart, m_dateEnd);
    setTabOrder(m_dateEnd, m_btnSearch);
    setTabOrder(m_btnSearch, m_btnReset);

    connect(m_btnSearch, &QPushButton::clicked, this, &KDicomQueueSearch::SlotToStartSearch);
    connect(m_btnReset, &QPushButton::clicked, this, &KDicomQueueSearch::SlotToResetSearchData);
    connect(m_dateStart, &KPatientDateEdit::dateChanged, this, &KDicomQueueSearch::OnStartDateChanged);
    connect(m_dateEnd, &KPatientDateEdit::dateChanged, this, &KDicomQueueSearch::OnEndDateChanged);
}

void KDicomQueueSearch::SlotToStartSearch()
{
    // Реф. @0x809028: собрать SQL-фрагменты, склеить " and ", emit {"Where": clause}.
    QStringList frags;
    if (!m_editPatientId->text().isEmpty())
        frags << QStringLiteral("PatientID like '%%%1%%'").arg(m_editPatientId->text());
    if (!m_editName->text().isEmpty())
        frags << QStringLiteral("PatientName like '%%%1%%'").arg(m_editName->text());
    if (m_cmbMsgType->currentIndex() > 1) {   // idx>1 (0 пусто, 1 All)
        static const char *codes[] = {"MPPS", "Store", "Commitment"};   // idx2..4
        frags << QStringLiteral("CommandType COLLATE NOCASE ='%1'")
                     .arg(QString::fromLatin1(codes[m_cmbMsgType->currentIndex() - 2]));
    }
    if (m_cmbStatus->currentIndex() > 1) {
        static const char *codes[] = {"Successful", "Fail", "In progress", "Waiting"};   // idx2..5
        frags << QStringLiteral("CommandStatus COLLATE NOCASE ='%1'")
                     .arg(QString::fromLatin1(codes[m_cmbStatus->currentIndex() - 2]));
    }
    if (m_dateStart->date() != KPatientDateEdit::InvalidDate())
        frags << QStringLiteral("LastUpdateDate >= '%1'").arg(m_dateStart->date().toString(kDateFmt));
    if (m_dateEnd->date() != KPatientDateEdit::InvalidDate())
        frags << QStringLiteral("LastUpdateDate <= '%1'").arg(m_dateEnd->date().toString(kDateFmt));

    if (frags.isEmpty())
        return;
    QMap<QString, QString> cond;
    cond.insert(QStringLiteral("Where"), frags.join(QStringLiteral(" and ")));
    emit QueryItems(cond);
}

void KDicomQueueSearch::SlotToResetSearchData()
{
    m_editPatientId->setText(QString());
    m_editName->setText(QString());
    m_cmbMsgType->setCurrentIndex(0);
    m_cmbStatus->setCurrentIndex(0);
    m_dateStart->clearMinimumDate();
    m_dateStart->clearMaximumDate();
    m_dateEnd->clearMinimumDate();
    m_dateEnd->clearMaximumDate();
    m_dateStart->setDate(KPatientDateEdit::InvalidDate());
    m_dateEnd->setDate(KPatientDateEdit::InvalidDate());
}

void KDicomQueueSearch::OnStartDateChanged(const QDate &d)
{
    if (d.isValid() && d != KPatientDateEdit::InvalidDate()
        && m_dateEnd->date() != KPatientDateEdit::InvalidDate())
        m_dateEnd->setMinimumDate(d);
}

void KDicomQueueSearch::OnEndDateChanged(const QDate &d)
{
    if (d.isValid() && d != KPatientDateEdit::InvalidDate()
        && m_dateStart->date() != KPatientDateEdit::InvalidDate())
        m_dateStart->setMaximumDate(d);
}

void KDicomQueueSearch::MoveFocusToFirstWidget()
{
    if (m_editPatientId)
        m_editPatientId->setFocus();
}
