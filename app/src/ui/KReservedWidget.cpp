#include "KReservedWidget.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDateEdit>

KReservedWidget::KReservedWidget(QWidget *parent, E_EDITOR_TYPE type)
    : QWidget(parent)
    , m_editorType(type)
{
    // Реф. ctor @0x4df450: QVBoxLayout(margins 9) + QLabel + вход по типу.
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(9, 9, 9, 9);
    m_label = new QLabel(this);
    m_layout->addWidget(m_label);

    switch (type) {
    case E_LINEEDIT:
        m_lineEdit = new QLineEdit(this);
        m_layout->addWidget(m_lineEdit);
        break;
    case E_DATEEDIT:
        m_dateEdit = new QDateEdit(this);
        m_dateEdit->setMinimumDate(QDate(1, 1, 1));
        m_layout->addWidget(m_dateEdit);
        break;
    default:
        break;   // только метка
    }
}

void KReservedWidget::SetLabel(const QString &text)
{
    if (m_label)
        m_label->setText(text);
}

void KReservedWidget::SetLineEditText(const QString &text)
{
    if (m_lineEdit)
        m_lineEdit->setText(text);
}

QString KReservedWidget::GetLineEditText() const
{
    return m_lineEdit ? m_lineEdit->text() : QString();
}

void KReservedWidget::SetMaxInputLen(int n)
{
    if (m_lineEdit)
        m_lineEdit->setMaxLength(n);
}

void KReservedWidget::SetDate(const QDate &date)
{
    // Реф.: валидная дата → setDate; sentinel/min → specialValueText(" ") + дата 0001-01-01.
    if (!m_dateEdit)
        return;
    if (date.isValid() && date > m_dateEdit->minimumDate()) {
        m_dateEdit->setDate(date);
    } else {
        m_dateEdit->setSpecialValueText(QStringLiteral(" "));   // «не задано» = один пробел
        m_dateEdit->setDate(QDate::fromString(QStringLiteral("0001-01-01"), QStringLiteral("yyyy-MM-dd")));
    }
}

QDate KReservedWidget::GetDate() const
{
    return m_dateEdit ? m_dateEdit->date() : QDate();
}

void KReservedWidget::installEventFilter(QObject *filter)
{
    // Реф.: форвард на активный вход (не на сам виджет).
    if (m_editorType == E_LINEEDIT && m_lineEdit)
        m_lineEdit->installEventFilter(filter);
    else if (m_editorType == E_DATEEDIT && m_dateEdit)
        m_dateEdit->installEventFilter(filter);
}
