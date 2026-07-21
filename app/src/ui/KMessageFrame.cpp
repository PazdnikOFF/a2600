#include "KMessageFrame.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QSizePolicy>
#include <QStringList>

KMessageFrame::KMessageFrame(QWidget *parent)
    : QFrame(parent)
{
    // Реф. ctor @0x682ab0: setupUi, m_startIndex=0.
    setupUi();
}

void KMessageFrame::setupUi()
{
    // Реф. Ui_KMessageFrame::setupUi @0x683750.
    setObjectName(QStringLiteral("KMessageFrame"));
    resize(370, 200);
    QSizePolicy sp(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sp.setHeightForWidth(sizePolicy().hasHeightForWidth());
    setSizePolicy(sp);

    QVBoxLayout *v = new QVBoxLayout(this);
    v->setObjectName(QStringLiteral("verticalLayout"));
    for (int i = 0; i < 5; ++i) {
        QLabel *l = new QLabel(this);
        l->setObjectName(QStringLiteral("label_message%1").arg(i + 1));
        QSizePolicy lp(QSizePolicy::Fixed, QSizePolicy::Expanding);   // реф. (0,7)
        lp.setHeightForWidth(l->sizePolicy().hasHeightForWidth());
        l->setSizePolicy(lp);
        l->setMinimumSize(360, 0);
        l->setText(QString());
        v->addWidget(l);
        m_labels[i] = l;
    }
}

void KMessageFrame::AddShowMessage(const QString &msg, int count)
{
    // Реф. @0x683020: дедуп по тексту, append, автоскролл к новейшим.
    for (int i = 0; i < m_messages.size(); ++i)
        if (m_messages[i].text == msg) {
            m_messages.removeAt(i);
            break;
        }
    m_messages.append({msg, count});
    if (m_messages.size() > m_startIndex + 4)
        m_startIndex = m_messages.size() - 4;
    UpdateShowMessage();
}

void KMessageFrame::ClearShowMessage(const QString &msg)
{
    // Реф. @0x683630: удалить запись == msg.
    for (int i = 0; i < m_messages.size(); ++i)
        if (m_messages[i].text == msg) {
            m_messages.removeAt(i);
            break;
        }
    UpdateShowMessage();
}

void KMessageFrame::UpdateShowMessage()
{
    // Реф. @0x682be0: с m_startIndex, split '\n', drop empty, префикс "-", разложить по 5 меткам.
    QStringList lines;
    for (int i = m_startIndex; i < m_messages.size() && lines.size() < 5; ++i) {
        const QStringList parts = m_messages[i].text.split(QLatin1Char('\n'));
        for (const QString &p : parts) {
            if (p.isEmpty())
                continue;
            lines << (QStringLiteral("-") + p);
            if (lines.size() >= 5)
                break;
        }
    }
    for (int i = 0; i < 5; ++i)
        m_labels[i]->setText(i < lines.size() ? lines[i] : QString());
}

void KMessageFrame::ShowMessageCheck()
{
    // Реф. @0x683228: countdown каждого сообщения, удалить count==0, скролл/сброс startIndex.
    for (int i = m_messages.size() - 1; i >= 0; --i) {
        if (--m_messages[i].count <= 0)
            m_messages.removeAt(i);
    }
    if (m_messages.size() > 5) {
        if (m_startIndex < m_messages.size() - 5)
            ++m_startIndex;
        else
            m_startIndex = 0;
    } else {
        m_startIndex = 0;
    }
    UpdateShowMessage();
}
