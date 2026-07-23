#include "KPatientListAddDlg.h"

#include <QComboBox>
#include <QDate>
#include <QDateEdit>

#include "KPatientDateEdit.h"
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegExpValidator>
#include <QWidget>

namespace {
// Контейнер поля (реф. widget_* — QWidget с абсолютной геометрией) + подпись сверху.
QWidget *fieldBox(QWidget *grp, const QRect &g, const QString &cap, const char *capName,
                  bool required = false)
{
    QWidget *w = new QWidget(grp);
    w->setGeometry(g);
    QLabel *l = new QLabel(w);
    l->setObjectName(QString::fromLatin1(capName));
    l->setGeometry(10, 3, 158, 21);
    if (required) {   // реф. RichText с красной «*»
        l->setTextFormat(Qt::RichText);
        l->setText(cap + QStringLiteral(" <font color=\"#e05050\">*</font>"));
    } else {
        l->setText(cap);
    }
    return w;
}
QLineEdit *lineIn(QWidget *box, const char *name, int maxLen = 0)
{
    QLineEdit *e = new QLineEdit(box);
    e->setObjectName(QString::fromLatin1(name));
    e->setGeometry(10, 30, 273, 31);
    if (maxLen > 0)
        e->setMaxLength(maxLen);
    return e;
}
// Editable-комбо с памятью (реф. KMemComboBox → editable QComboBox + maxLength).
QComboBox *memCombo(QWidget *box, const char *name, int maxLen)
{
    QComboBox *c = new QComboBox(box);
    c->setObjectName(QString::fromLatin1(name));
    c->setGeometry(10, 30, 273, 31);
    c->setEditable(true);
    if (c->lineEdit())
        c->lineEdit()->setMaxLength(maxLen);
    return c;
}
} // namespace

KPatientListAddDlg::KPatientListAddDlg(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x7c2a60: KDialog(modal=false) → setupUi → SetKStyle(7) → title TR_APatient →
    // InitWidget → IsPatient*WidgetHidden (device) → Initconnect → InitWidgetFileter.
    setupUi();
    SetKStyle(KDLG_W1024);             // реф. SetKStyle(7)
    SetTitle(tr("TR_APatient"));
}

void KPatientListAddDlg::setupUi()
{
    setObjectName(QStringLiteral("KPatientListAddDlg"));

    QWidget *host = ContentArea();
    host->setFixedSize(1024, 712);     // реф. фикс. размер 1024×712

    // ===================== Группа «инфо пациента» =====================
    QGroupBox *grpPat = new QGroupBox(host);
    grpPat->setObjectName(QStringLiteral("grp_patientinfo"));
    grpPat->setGeometry(30, 49, 972, 291);

    // Ряд 1: ID / имя(*) / пол
    QWidget *wId = fieldBox(grpPat, QRect(7, 22, 291, 71), tr("TR_PID2"), "label_patientid");
    memCombo(wId, "cmb_patientid", 32);
    QWidget *wName = fieldBox(grpPat, QRect(329, 22, 291, 71), tr("TR_Nme:"), "label_name", true);
    memCombo(wName, "cmb_name", 50);
    QWidget *wGdr = fieldBox(grpPat, QRect(650, 22, 291, 71), tr("TR_Gdr:"), "label_gender");
    QComboBox *cmbGender = new QComboBox(wGdr);
    cmbGender->setObjectName(QStringLiteral("cmb_gender"));
    cmbGender->setGeometry(10, 30, 273, 31);
    cmbGender->addItem(QString());               // реф. s_vct_gender: пусто/М/Ж/неизв.
    cmbGender->addItem(tr("TR_M"));
    cmbGender->addItem(tr("TR_F"));
    cmbGender->addItem(tr("TR_Nknwn"));

    // Ряд 2: ДР(+календарь) / возраст / телефон
    QWidget *wDob = fieldBox(grpPat, QRect(7, 110, 291, 71), tr("TR_DoB:"), "label_dob");
    // Реф. Ui_KPatientListAddDlg::setupUi: поле даты рождения — KPatientDateEdit
    // (плейсхолдер вместо пустой даты, сентинел QDate(2100,1,1)), а не голый QDateEdit.
    m_dateDob = new KPatientDateEdit(wDob);
    m_dateDob->setObjectName(QStringLiteral("date_dob"));
    m_dateDob->setGeometry(10, 30, 240, 31);
    m_dateDob->setCalendarPopup(true);
    m_dateDob->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    m_dateDob->setDate(QDate(2000, 1, 1));
    QPushButton *btnDob = new QPushButton(wDob);
    btnDob->setObjectName(QStringLiteral("btn_dob"));
    btnDob->setGeometry(252, 32, 26, 26);
    connect(btnDob, &QPushButton::clicked, m_dateDob, [this]() { m_dateDob->setFocus(); });
    QWidget *wAge = fieldBox(grpPat, QRect(329, 110, 291, 71), tr("TR_Age:"), "label_age");
    m_editAge = lineIn(wAge, "edit_age");
    m_editAge->setInputMethodHints(Qt::ImhPreferNumbers);
    m_editAge->setValidator(new QRegExpValidator(
        QRegExp(QStringLiteral("^([1-9]|[1-9]\\d|(1[0-9][0-9]))$")), m_editAge));  // 1..199
    QWidget *wTel = fieldBox(grpPat, QRect(650, 110, 291, 71), tr("TR_Tel:"), "label_phone");
    QLineEdit *edPhone = lineIn(wTel, "edit_phone", 32);
    edPhone->setInputMethodHints(Qt::ImhPreferNumbers);
    edPhone->setValidator(new QRegExpValidator(
        QRegExp(QStringLiteral("^[0-9\\-]{1,32}$")), edPhone));

    // Ряд 3: койка / польз-поле 1 / польз-поле 2
    QWidget *wBed = fieldBox(grpPat, QRect(7, 200, 291, 71), tr("TR_BNo:"), "label_bedno");
    lineIn(wBed, "edit_bedno", 32);
    QWidget *wU1 = fieldBox(grpPat, QRect(329, 200, 291, 71), tr("TR_CField1"), "label_useritem1");
    lineIn(wU1, "edit_useritem1", 32);
    QWidget *wU2 = fieldBox(grpPat, QRect(650, 200, 291, 71), tr("TR_CField2"), "label_useritem2");
    lineIn(wU2, "edit_useritem2", 32);

    // Реф. взаимный пересчёт возраст↔ДР (чистый UI, с блокировкой сигналов от петли).
    connect(m_editAge, &QLineEdit::textChanged, this, [this](const QString &t) {
        bool ok = false; int age = t.toInt(&ok);
        if (!ok) return;
        QDate d = QDate(QDate::currentDate().year() - age,
                        m_dateDob->date().month(), m_dateDob->date().day());
        if (!d.isValid()) d = QDate(QDate::currentDate().year() - age, 1, 1);
        QSignalBlocker b(m_dateDob);
        m_dateDob->setDate(d);
    });
    connect(m_dateDob, &QDateEdit::dateChanged, this, [this](const QDate &d) {
        int age = QDate::currentDate().year() - d.year();
        QSignalBlocker b(m_editAge);
        m_editAge->setText(QString::number(age));
    });

    // ===================== Группа «инфо обследования» =====================
    QGroupBox *grpExam = new QGroupBox(host);
    grpExam->setObjectName(QStringLiteral("grp_examinfo"));
    grpExam->setGeometry(30, 370, 972, 201);

    QWidget *wItem = fieldBox(grpExam, QRect(8, 25, 291, 71), tr("TR_EItem:"), "label_examitem", true);
    QComboBox *cmbItem = new QComboBox(wItem);   // реф. device: GetEndoClassToQstring
    cmbItem->setObjectName(QStringLiteral("cmb_examitem"));
    cmbItem->setGeometry(10, 30, 273, 31);
    QWidget *wPlan = fieldBox(grpExam, QRect(339, 25, 291, 71), tr("TR_PDate:"), "label_plantime");
    QDateEdit *datePlan = new QDateEdit(wPlan);
    datePlan->setObjectName(QStringLiteral("date_plantime"));
    datePlan->setGeometry(10, 30, 240, 31);
    datePlan->setCalendarPopup(true);
    datePlan->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    datePlan->setDate(QDate::currentDate());
    QPushButton *btnPlan = new QPushButton(wPlan);
    btnPlan->setObjectName(QStringLiteral("btn_plantime"));
    btnPlan->setGeometry(257, 32, 26, 26);
    connect(btnPlan, &QPushButton::clicked, datePlan, [datePlan]() { datePlan->setFocus(); });
    QWidget *wAppl = fieldBox(grpExam, QRect(8, 109, 291, 71), tr("TR_Aplct:"), "label_applicant");
    memCombo(wAppl, "cmb_applicant", 32);
    QWidget *wApplT = fieldBox(grpExam, QRect(339, 109, 291, 71), tr("TR_ADate:"), "label_applytime");
    QDateEdit *dateApply = new QDateEdit(wApplT);
    dateApply->setObjectName(QStringLiteral("date_applytime"));
    dateApply->setGeometry(10, 30, 273, 31);
    dateApply->setCalendarPopup(true);
    dateApply->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    dateApply->setDate(QDate::currentDate());

    // ===================== Подсказка + кнопки =====================
    QLabel *tip = new QLabel(host);
    tip->setObjectName(QStringLiteral("label_tip"));
    tip->setGeometry(28, 597, 971, 25);
    tip->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    tip->setText(tr("TR_RField"));

    QPushButton *btnExam = new QPushButton(tr("TR_Exm"), host);
    btnExam->setObjectName(QStringLiteral("btn_exam"));
    btnExam->setGeometry(280, 641, 133, 40);   // реф. OnBtnExam (device) — не подключаем
    QPushButton *btnOk = new QPushButton(tr("TR_OK"), host);
    btnOk->setObjectName(QStringLiteral("btn_confirm"));
    btnOk->setGeometry(445, 641, 133, 40);     // реф. OnBtnConfirm (device) — не подключаем
    QPushButton *btnCancel = new QPushButton(tr("TR_Ccl"), host);
    btnCancel->setObjectName(QStringLiteral("btn_cancel"));
    btnCancel->setGeometry(612, 641, 133, 40);
    connect(btnCancel, &QPushButton::clicked, this, &QWidget::close);   // реф. OnBtnCancel→close
}
