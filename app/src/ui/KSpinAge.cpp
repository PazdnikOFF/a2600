#include "KSpinAge.h"

#include <QLineEdit>
#include <QKeyEvent>

KSpinAge::KSpinAge(QWidget *parent)
    : QSpinBox(parent)
{
    // Реф. ctor @0x6904b8: range 1..199, стартует пустым, коннекты валидации.
    connect(lineEdit(), &QLineEdit::textChanged, this, &KSpinAge::CheckText);
    connect(lineEdit(), &QLineEdit::editingFinished, this, &KSpinAge::EditFinished);
    setRange(1, 199);
    clear();
    m_flag = true;
}

int KSpinAge::value() const
{
    // Реф. @0x690700: пустой lineEdit → 0.
    if (lineEdit()->text().isEmpty())
        return 0;
    return QSpinBox::value();
}

void KSpinAge::setValue(int v)
{
    // Реф. @0x6906c0: 0 → clear, иначе база.
    if (v == 0) {
        clear();
    } else {
        QSpinBox::setValue(v);
        m_flag = false;
    }
}

void KSpinAge::SetPlaceholderText(const QString &t)
{
    lineEdit()->setPlaceholderText(t);
}

void KSpinAge::CheckText(const QString &text)
{
    // Реф. @0x690598: пусто&&n<=0 → clear; n<0 → выход; иначе значение принято.
    const int n = text.toInt();
    if (!text.isEmpty() && n <= 0)
        clear();
}

void KSpinAge::EditFinished()
{
    // Реф. @0x690618: перепроверить; при взведённом флаге — clear.
    CheckText(lineEdit()->text());
    if (m_flag) {
        clear();
        m_flag = false;
    }
}

void KSpinAge::keyPressEvent(QKeyEvent *e)
{
    // Реф. @0x6908e0: Enter в пустом поле → clear.
    if ((e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) && lineEdit()->text().isEmpty())
        clear();
    QSpinBox::keyPressEvent(e);
}
