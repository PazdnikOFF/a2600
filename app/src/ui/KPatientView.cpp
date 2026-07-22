#include "KPatientView.h"
#include "KMemComboBox.h"
#include "KSpinAge.h"
#include "KEmpDateEdit.h"
#include "Theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPixmap>

KPatientView::KPatientView(QWidget *parent)
    : QWidget(parent)
{
    // Реф. ctor: строит поля (фикс-ширина 178) + иконки. KObject-шина/collapse — опущены.
    setObjectName(QStringLiteral("KPatientView"));

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(6);

    m_name = new KMemComboBox(this);
    m_name->setEditable(true);
    if (m_name->lineEdit()) m_name->lineEdit()->setPlaceholderText(tr("TR_Nme"));
    root->addWidget(makeRow(QStringLiteral("patient_name"), m_name));

    m_gender = new QComboBox(this);
    m_gender->addItem(QString(), 3);            // blank
    m_gender->addItem(tr("TR_M"), 1);           // Male
    m_gender->addItem(tr("TR_F"), 0);           // Female
    m_gender->addItem(tr("TR_Nknwn"), 2);       // Unknown
    root->addWidget(makeRow(QStringLiteral("patient_sex"), m_gender));

    m_age = new KSpinAge(this);
    root->addWidget(makeRow(QStringLiteral("patient_age"), m_age));

    m_dob = new KEmpDateEdit(this);
    m_dob->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));   // реф. KSystemSet date fmt (стаб)
    m_dob->setMaximumDate(QDate::currentDate());
    m_dob->SetEmptyAble(true);
    m_dob->SetPlaceholderText(tr("TR_DoB"));
    root->addWidget(makeRow(QStringLiteral("patient_birthday"), m_dob));

    m_applicant = new KMemComboBox(this);
    m_applicant->setEditable(true);
    if (m_applicant->lineEdit()) m_applicant->lineEdit()->setPlaceholderText(tr("TR_Aplct"));
    root->addWidget(makeRow(QStringLiteral("applicant"), m_applicant));

    m_patientId = new KMemComboBox(this);
    m_patientId->setEditable(true);
    if (m_patientId->lineEdit()) m_patientId->lineEdit()->setPlaceholderText(tr("TR_PID"));
    root->addWidget(makeRow(QStringLiteral("patient_id"), m_patientId));

    m_examNo = new QLineEdit(this);
    m_examNo->setReadOnly(true);
    m_examNo->setPlaceholderText(tr("TR_ENo"));
    root->addWidget(makeRow(QStringLiteral("exam_no"), m_examNo));

    m_custom1 = new QLineEdit(this);
    m_custom1->setPlaceholderText(tr("TR_CField1"));
    root->addWidget(makeRow(QStringLiteral("custom_item"), m_custom1));

    m_custom2 = new QLineEdit(this);
    m_custom2->setPlaceholderText(tr("TR_CField2"));
    root->addWidget(makeRow(QStringLiteral("custom_item"), m_custom2));

    // Doctor — реф. bit 0x10 скрыт в этой сборке; создаём, но не показываем.
    m_doctor = new QLineEdit(this);
    m_doctor->setReadOnly(true);
    m_doctor->hide();

    root->addStretch(1);
}

QWidget *KPatientView::makeRow(const QString &iconName, QWidget *editor)
{
    // Реф. SetItemIcon: QLabel-иконка 28×28 из qss/black/icon/patient/ слева от редактора.
    QWidget *row = new QWidget(this);
    QHBoxLayout *h = new QHBoxLayout(row);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(6);
    QLabel *icon = new QLabel(row);
    icon->setFixedSize(28, 28);
    icon->setScaledContents(true);
    icon->setPixmap(QPixmap(theme::asset(QStringLiteral("black/icon/patient/") + iconName + QStringLiteral(".png"))));
    h->addWidget(icon);
    editor->setFixedWidth(178);   // реф. 0xb2
    h->addWidget(editor);
    h->addStretch(1);
    return row;
}

void KPatientView::setPatient(const KPatientInfo &info)
{
    // Реф. LoadPatientInfo: struct → виджеты (маппинг §5).
    m_name->setEditText(info.name);
    if (info.genderCode == 3) m_gender->setCurrentIndex(0);
    else { int i = m_gender->findData(info.genderCode); m_gender->setCurrentIndex(i < 0 ? 0 : i); }
    m_age->setValue(info.age);
    m_examNo->setText(info.examNo);
    m_patientId->setEditText(info.patientId);
    m_applicant->setEditText(info.applicant);
    if (!info.dob.isEmpty()) {
        const QDate d = QDate::fromString(info.dob, QStringLiteral("yyyy-MM-dd"));
        if (d.isValid()) m_dob->setDate(d);
    }
    m_custom1->setText(info.custom1);
    m_custom2->setText(info.custom2);
    m_doctor->setText(info.doctor);
}

KPatientInfo KPatientView::GetPatient() const
{
    // Реф. SavePatientData: чтение полей → struct (examNo/doctor не пишутся).
    KPatientInfo info;
    info.name = m_name->currentText().trimmed();
    info.genderCode = m_gender->currentIndex() < 0 ? 3 : m_gender->currentData().toInt();
    info.age = m_age->value();
    info.patientId = m_patientId->currentText().trimmed();
    info.dob = m_dob->date().toString(QStringLiteral("yyyy-MM-dd"));
    info.applicant = m_applicant->currentText().trimmed();
    info.custom1 = m_custom1->text().trimmed();
    info.custom2 = m_custom2->text().trimmed();
    return info;
}

void KPatientView::ClearWidgetInfo()
{
    // Реф. @ClearWidgetInfo: сброс всех редакторов.
    m_name->setEditText(QString());
    m_gender->setCurrentIndex(0);
    m_age->setValue(0);
    m_dob->clear();
    m_applicant->setEditText(QString());
    m_patientId->setEditText(QString());
    m_examNo->clear();
    m_custom1->clear();
    m_custom2->clear();
    m_doctor->clear();
}
