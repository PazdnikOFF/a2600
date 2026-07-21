#include "KExamListSearch.h"
#include "KMemComboBox.h"
#include "KPatientDateEdit.h"

#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDate>

static const QString kDateFmt = QStringLiteral("yyyy-MM-dd");   // реф. KSystemSet::GetDateFormatString (device)

KExamListSearch::KExamListSearch(QWidget *parent)
    : QWidget(parent)
{
    // Реф. ctor @0x7f42d8: setupUi → InitWidget → InitConnect → InitWidgetFileter.
    setupUi();
}

void KExamListSearch::setupUi()
{
    setObjectName(QStringLiteral("KExamListSearch"));

    auto mkLbl = [&](QWidget *p, const QString &t, const char *name) {
        QLabel *l = new QLabel(t, p);
        l->setObjectName(QString::fromLatin1(name));
        return l;
    };
    // Ячейка: метка сверху, поле снизу (как у сиблинга KPatientListSearch).
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

    // Врач — РЕАЛЬНЫЙ KMemComboBox (DB-history tb_QuickInputDoctor).
    m_cmbDoctor = new KMemComboBox(this);
    m_cmbDoctor->setObjectName(QStringLiteral("cmb_doctor"));
    m_cmbDoctor->setMaxLength(50);
    m_cmbDoctor->SetTableName(QStringLiteral("tb_QuickInputDoctor"), true);   // DB-seam
    g->addWidget(cell(tr("TR_Dctr:"), "label_doctor", m_cmbDoctor), 0, 2);

    // Статус — enum жизненного цикла осмотра (ReportStatus).
    m_cmbStatus = new QComboBox(this);
    m_cmbStatus->setObjectName(QStringLiteral("cmb_status"));
    m_cmbStatus->addItem(QString());   // пустой первый
    for (const char *k : {"TR_All", "TR_EXMING", "TR_Exmed", "TR_Cexam",
                          "TR_Dgnsed", "TR_Pnted", "TR_Uplded", "TR_PUploaded"})
        m_cmbStatus->addItem(tr(k));
    m_cmbStatus->setCurrentIndex(0);
    g->addWidget(cell(tr("TR_Stts:"), "label_status", m_cmbStatus), 0, 3);

    // Диапазон дат осмотра — 2 РЕАЛЬНЫХ KPatientDateEdit + кнопки календаря.
    auto mkDate = [&](const char *name, const char *btnName) {
        QWidget *w = new QWidget(this);
        QHBoxLayout *h = new QHBoxLayout(w);
        h->setContentsMargins(0, 0, 0, 0);
        KPatientDateEdit *d = new KPatientDateEdit(w);
        d->setObjectName(QString::fromLatin1(name));
        d->setDisplayFormat(kDateFmt);
        d->setDate(KPatientDateEdit::InvalidDate());   // пусто/плейсхолдер
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
    vDS->addWidget(mkLbl(wDS, tr("TR_EmDate:"), "label_date"));
    vDS->addWidget(ds.first);
    g->addWidget(wDS, 1, 0, 1, 2);

    auto de = mkDate("dateEdit_end", "btnEndDate");
    m_dateEnd = de.second;
    QWidget *wDE = new QWidget(this);
    QVBoxLayout *vDE = new QVBoxLayout(wDE); vDE->setContentsMargins(0, 0, 0, 0);
    vDE->addWidget(mkLbl(wDE, QStringLiteral("--"), "label_spar"));   // сепаратор диапазона
    vDE->addWidget(de.first);
    g->addWidget(wDE, 1, 2);

    // Кнопки поиск/сброс.
    QVBoxLayout *vBtn = new QVBoxLayout();
    m_btnSearch = new QPushButton(tr("TR_Sch") + QStringLiteral("(F11)"), this);
    m_btnSearch->setObjectName(QStringLiteral("btn_search"));
    m_btnReset = new QPushButton(tr("TR_Rst2") + QStringLiteral("(F12)"), this);
    m_btnReset->setObjectName(QStringLiteral("btn_reset"));
    vBtn->addWidget(m_btnSearch);
    vBtn->addWidget(m_btnReset);
    g->addLayout(vBtn, 1, 3);

    // Tab-порядок (реф.).
    setTabOrder(m_editPatientId, m_editName);
    setTabOrder(m_editName, m_cmbDoctor);
    setTabOrder(m_cmbDoctor, m_cmbStatus);
    setTabOrder(m_cmbStatus, m_dateStart);
    setTabOrder(m_dateStart, m_dateEnd);
    setTabOrder(m_dateEnd, m_btnSearch);
    setTabOrder(m_btnSearch, m_btnReset);

    // Реф. InitConnect.
    connect(m_btnSearch, &QPushButton::clicked, this, &KExamListSearch::SlotToStartSearch);
    connect(m_btnReset, &QPushButton::clicked, this, &KExamListSearch::SlotToResetSearchData);
    connect(m_dateStart, &KPatientDateEdit::dateChanged, this, &KExamListSearch::OnStartDateChanged);
    connect(m_dateEnd, &KPatientDateEdit::dateChanged, this, &KExamListSearch::OnEndDateChanged);
}

void KExamListSearch::SlotToStartSearch()
{
    // Реф. @0x7f4788: собрать map условий → emit QueryItems (DB-seam).
    QMap<QString, QString> cond;
    if (!m_editPatientId->text().isEmpty())
        cond.insert(QStringLiteral("PatientID"),
                    QStringLiteral("like '%%%1%%'").arg(m_editPatientId->text()));
    if (!m_editName->text().isEmpty())
        cond.insert(QStringLiteral("PatientName"),
                    QStringLiteral("like '%%%1%%'").arg(m_editName->text()));
    if (!m_cmbDoctor->text().isEmpty())
        cond.insert(QStringLiteral("DrExamName"),
                    QStringLiteral("like '%%%1%%'").arg(m_cmbDoctor->text()));
    if (m_cmbStatus->currentIndex() > 0)
        cond.insert(QStringLiteral("ReportStatus"), QString::number(m_cmbStatus->currentIndex()));
    if (m_dateStart->date() != KPatientDateEdit::InvalidDate())
        cond.insert(QStringLiteral("ExamDate>="),
                    m_dateStart->date().toString(kDateFmt));
    if (m_dateEnd->date() != KPatientDateEdit::InvalidDate())
        cond.insert(QStringLiteral("ExamDate<="),
                    m_dateEnd->date().toString(kDateFmt));
    emit QueryItems(cond);
}

void KExamListSearch::SlotToResetSearchData()
{
    // Реф.: очистить все поля.
    m_editPatientId->setText(QString());
    m_editName->setText(QString());
    m_cmbDoctor->setText(QString());
    m_cmbStatus->setCurrentIndex(0);
    m_dateStart->setDate(KPatientDateEdit::InvalidDate());
    m_dateEnd->setDate(KPatientDateEdit::InvalidDate());
}

void KExamListSearch::OnStartDateChanged(const QDate &d)
{
    // Реф.: валидная start-дата ограничивает минимум end (from ≤ to). НО ограничиваем ТОЛЬКО
    // если end уже содержит реальную дату — иначе setMinimumDate собьёт пустой end с
    // плейсхолдера (KPatientDateEdit держит пусто на minimumDate).
    if (d.isValid() && d != KPatientDateEdit::InvalidDate()
        && m_dateEnd->date() != KPatientDateEdit::InvalidDate())
        m_dateEnd->setMinimumDate(d);
}

void KExamListSearch::OnEndDateChanged(const QDate &d)
{
    if (d.isValid() && d != KPatientDateEdit::InvalidDate()
        && m_dateStart->date() != KPatientDateEdit::InvalidDate())
        m_dateStart->setMaximumDate(d);
}

void KExamListSearch::MoveFocusToFirstWidget()
{
    if (m_editPatientId)
        m_editPatientId->setFocus();
}
