#include "KFloatingMsg.h"

#include <QVBoxLayout>
#include <QLabel>

KFloatingMsg::KFloatingMsg(QWidget *parent)
    : QWidget(parent)
{
    // Реф. ctor @0x68fb48.
    setObjectName(QStringLiteral("KFloatingMsg"));
    m_layout = new QVBoxLayout(this);
    m_layout->setObjectName(QStringLiteral("verticalLayout"));
    m_label = new QLabel(this);
    m_label->setObjectName(QStringLiteral("label_FloatingM"));
    m_label->setAlignment(Qt::AlignCenter);
    m_layout->addWidget(m_label);
    setWindowTitle(tr("TR_Fm"));
    m_label->setText(QString());
    setVisible(false);
    setStyleSheet(QStringLiteral("background:black"));
}

void KFloatingMsg::AddText(const QString &text)
{
    if (!m_textList.contains(text)) {
        m_textList.append(text);
        ShowText();
    }
}

void KFloatingMsg::DelText(const QString &text)
{
    m_textList.removeAll(text);
    ShowText();
}

void KFloatingMsg::SetTextShow(const QString &text, bool show)
{
    if (show)
        AddText(text);
    else
        DelText(text);
}

void KFloatingMsg::ClearTextShow()
{
    m_textList.clear();
    ShowText();
}

void KFloatingMsg::ShowText()
{
    // Реф. @0x68f5c0: join «\r\n», пусто → «».
    m_label->setText(m_textList.join(QStringLiteral("\r\n")));
    setVisible(!m_textList.isEmpty());
}

void KFloatingMsg::SetFloatingRect(const QRect &rect)
{
    move(rect.topLeft());
    resize(rect.size());
}
