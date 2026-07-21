#include "KPatientListEditDlg.h"
#include "KMemComboBox.h"
#include "KPatientDateEdit.h"

#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QDateEdit>
#include <QGroupBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRegExpValidator>
#include <QDate>

static const QString kDateFmt = QStringLiteral("yyyy-MM-dd");

KPatientListEditDlg::KPatientListEditDlg(const QString &patientKey, QWidget *parent)
    : KDialog(parent, false)
    , m_patientKey(patientKey)
{
    // Реф. ctor @0x7cf010: setupUi → SetKStyle(7) → title → LoadPatientData(device) →
    // InitWidget → Initconnect.
    SetKStyle(KDLG_W1024);   // реф. SetKStyle(7)
    SetTitle(tr("TR_EPatient"));
    buildUi();

    // Реф. Initconnect: ДР↔возраст + кнопки.
    connect(m_dateDob, &KPatientDateEdit::dateChanged, this, &KPatientListEditDlg::OnChangedPatientDate);
    connect(m_editAge, &QLineEdit::textChanged, this, &KPatientListEditDlg::OnChangedPatientAge);
}

void KPatientListEditDlg::buildUi()
{
    setObjectName(QStringLiteral("KPatientListEditDlg"));
    QWidget *content = ContentArea();
    QVBoxLayout *outer = new QVBoxLayout(content);

    auto cell = [&](const QString &cap, const char *capName, QWidget *field) {
        QWidget *w = new QWidget(content);
        QVBoxLayout *v = new QVBoxLayout(w);
        v->setContentsMargins(4, 2, 4, 2);
        QLabel *l = new QLabel(cap, w);
        l->setObjectName(QString::fromLatin1(capName));
        v->addWidget(l);
        v->addWidget(field);
        return w;
    };
    auto mkEdit = [&](const char *name) {
        QLineEdit *e = new QLineEdit(content);
        e->setObjectName(QString::fromLatin1(name));
        return e;
    };
    auto mkDate = [&](const char *name, const char *btnName, const QDate &maxDate) {
        QWidget *w = new QWidget(content);
        QHBoxLayout *h = new QHBoxLayout(w);
        h->setContentsMargins(0, 0, 0, 0);
        KPatientDateEdit *d = new KPatientDateEdit(w);
        d->setObjectName(QString::fromLatin1(name));
        d->setDisplayFormat(kDateFmt);
        if (maxDate.isValid())
            d->setMaximumDate(maxDate);
        d->setDate(KPatientDateEdit::InvalidDate());
        QPushButton *b = new QPushButton(w);   // календарь-попап (реф. KCalendarWidget — seam)
        b->setObjectName(QString::fromLatin1(btnName));
        b->setFixedWidth(24);
        h->addWidget(d); h->addWidget(b);
        return qMakePair(w, d);
    };

    // ── grp_patientinfo (3×3) ──
    QGroupBox *grpP = new QGroupBox(content);
    grpP->setObjectName(QStringLiteral("grp_patientinfo"));
    QGridLayout *gp = new QGridLayout(grpP);

    m_cmbPatientId = new KMemComboBox(content);
    m_cmbPatientId->setObjectName(QStringLiteral("cmb_patientid"));
    m_cmbPatientId->SetTableName(QStringLiteral("tb_QuickInputPatient"), true);
    gp->addWidget(cell(tr("TR_PID2"), "label_patientid", m_cmbPatientId), 0, 0);

    m_cmbName = new KMemComboBox(content);
    m_cmbName->setObjectName(QStringLiteral("cmb_name"));
    m_cmbName->SetTableName(QStringLiteral("tb_QuickInputPatient"), true);
    gp->addWidget(cell(tr("TR_Nme:"), "label_name", m_cmbName), 0, 1);

    m_cmbGender = new QComboBox(content);
    m_cmbGender->setObjectName(QStringLiteral("cmb_gender"));
    m_cmbGender->addItem(QString(), 3);          // пусто
    m_cmbGender->addItem(tr("TR_M"), 1);         // муж
    m_cmbGender->addItem(tr("TR_F"), 0);         // жен
    m_cmbGender->addItem(tr("TR_Nknwn"), 2);     // неизв
    gp->addWidget(cell(tr("TR_Gdr:"), "label_gender", m_cmbGender), 0, 2);

    auto dob = mkDate("date_dob", "btn_dob", QDate());
    m_dateDob = dob.second;
    gp->addWidget(cell(tr("TR_DoB:"), "label_dob", dob.first), 1, 0);

    m_editAge = mkEdit("edit_age");
    m_editAge->setValidator(new QRegExpValidator(
        QRegExp(QStringLiteral("^([1-9]|[1-9]\\d|(1[0-9][0-9]))$")), m_editAge));   // 1–199
    gp->addWidget(cell(tr("TR_Age:"), "label_age", m_editAge), 1, 1);

    m_editPhone = mkEdit("edit_phone");
    m_editPhone->setValidator(new QRegExpValidator(
        QRegExp(QStringLiteral("[0-9\\-]{1,32}$")), m_editPhone));
    m_editPhone->setMaxLength(32);
    gp->addWidget(cell(tr("TR_Tel:"), "label_phone", m_editPhone), 1, 2);

    m_editBedno = mkEdit("edit_bedno");
    gp->addWidget(cell(tr("TR_BNo:"), "label_bedno", m_editBedno), 2, 0);
    gp->addWidget(cell(tr("TR_CField1"), "label_useritem1", mkEdit("edit_useritem1")), 2, 1);
    gp->addWidget(cell(tr("TR_CField2"), "label_useritem2", mkEdit("edit_useritem2")), 2, 2);
    outer->addWidget(grpP);

    // ── grp_examinfo (2×2) ──
    QGroupBox *grpE = new QGroupBox(content);
    grpE->setObjectName(QStringLiteral("grp_examinfo"));
    QGridLayout *ge = new QGridLayout(grpE);

    m_cmbExamItem = new QComboBox(content);
    m_cmbExamItem->setObjectName(QStringLiteral("cmb_examitem"));
    m_cmbExamItem->addItems({tr("TR_Gstrscpy"), tr("TR_Clnscpy"), tr("TR_Brnchscpy")});
    ge->addWidget(cell(tr("TR_EItem:"), "label_examitem", m_cmbExamItem), 0, 0);

    auto plan = mkDate("date_plantime", "btn_plantime", QDate(2099, 12, 31));
    m_datePlantime = plan.second;
    ge->addWidget(cell(tr("TR_PDate:"), "label_plantime", plan.first), 0, 1);

    m_cmbApplicant = new KMemComboBox(content);
    m_cmbApplicant->setObjectName(QStringLiteral("cmb_applicant"));
    m_cmbApplicant->SetTableName(QStringLiteral("tb_QuickInputApplicant"), true);
    ge->addWidget(cell(tr("TR_Aplct:"), "label_applicant", m_cmbApplicant), 1, 0);

    m_dateApplytime = new QDateEdit(content);   // реф. plain QDateEdit, min=max=today
    m_dateApplytime->setObjectName(QStringLiteral("date_applytime"));
    m_dateApplytime->setDisplayFormat(kDateFmt);
    m_dateApplytime->setDate(QDate::currentDate());
    m_dateApplytime->setMinimumDate(QDate::currentDate());
    m_dateApplytime->setMaximumDate(QDate::currentDate());
    ge->addWidget(cell(tr("TR_ADate:"), "label_applytime", m_dateApplytime), 1, 1);
    outer->addWidget(grpE);

    // ── подсказка + кнопки ──
    QLabel *tip = new QLabel(tr("TR_RField"), content);
    tip->setObjectName(QStringLiteral("label_tip"));
    outer->addWidget(tip);

    QHBoxLayout *btns = new QHBoxLayout();
    btns->addStretch();
    QPushButton *btnExam = new QPushButton(tr("TR_Exm"), content);
    btnExam->setObjectName(QStringLiteral("btn_exam"));
    QPushButton *btnOk = new QPushButton(tr("TR_OK"), content);
    btnOk->setObjectName(QStringLiteral("btn_confirm"));
    QPushButton *btnCancel = new QPushButton(tr("TR_Ccl"), content);
    btnCancel->setObjectName(QStringLiteral("btn_cancel"));
    btns->addWidget(btnExam); btns->addWidget(btnOk); btns->addWidget(btnCancel);
    outer->addLayout(btns);

    connect(btnOk, &QPushButton::clicked, this, &KPatientListEditDlg::OnBtnConfirm);
    connect(btnExam, &QPushButton::clicked, this, &KPatientListEditDlg::OnBtnConfirm);
    connect(btnCancel, &QPushButton::clicked, this, &KPatientListEditDlg::OnBtnCancel);
}

int KPatientListEditDlg::GetAge(const QDate &dob) const
{
    // Реф. @0x7ca090: целые годы до сегодня.
    if (!dob.isValid())
        return 0;
    const QDate today = QDate::currentDate();
    int age = today.year() - dob.year();
    if (today.month() < dob.month() || (today.month() == dob.month() && today.day() < dob.day()))
        --age;
    return age;
}

void KPatientListEditDlg::OnChangedPatientDate(const QDate &dob)
{
    // Реф. @0x7ca218: ДР → возраст. Пусто/невалид → выход. Иначе записать GetAge в поле.
    if (!dob.isValid() || dob == KPatientDateEdit::InvalidDate())
        return;
    const int age = GetAge(dob);
    const QString shown = (age < 1) ? QString() : QString::number(age);
    if (m_editAge->text() != shown)
        m_editAge->setText(shown);   // при совпадении возраста OnChangedPatientAge не сбросит ДР
}

void KPatientListEditDlg::OnChangedPatientAge()
{
    // Реф. @0x7ca138: если введённый возраст ≠ GetAge(ДР) — сбросить ДР в InvalidDate.
    const int typed = m_editAge->text().trimmed().toInt();
    if (m_dateDob->date() != KPatientDateEdit::InvalidDate() && typed != GetAge(m_dateDob->date()))
        m_dateDob->setDate(KPatientDateEdit::InvalidDate());
}

void KPatientListEditDlg::OnBtnConfirm()
{
    // Реф.: SavePatientInfo (DB-seam) + PublishMsg + close. В порте — только close.
    close();
}

void KPatientListEditDlg::OnBtnCancel()
{
    close();
}
