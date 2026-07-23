#include "KImgListCell.h"

#include <QLabel>
#include <QVBoxLayout>

// Реф. резерв высоты под подпись (`sub w2, w2, #0x14`).
static const int kTextReserve = 20;

KImgListCell::KImgListCell(QWidget *parent)
    : QFrame(parent)
{
    // Реф. ctor @0x6820f8 (setupUi заинлайнен, порядок сохранён).
    if (objectName().isEmpty())
        setObjectName(QStringLiteral("KImgListCell"));
    resize(400, 300);
    setFrameShape(QFrame::StyledPanel);   // реф. 6
    setFrameShadow(QFrame::Raised);       // реф. 0x20

    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(0);
    m_layout->setObjectName(QStringLiteral("verticalLayout"));
    m_layout->setContentsMargins(0, 0, 0, 0);

    m_labelImg = new QLabel(this);
    m_labelImg->setObjectName(QStringLiteral("label_img"));
    m_labelImg->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));
    m_labelImg->setAlignment(Qt::AlignCenter);   // реф. 0x84
    m_layout->addWidget(m_labelImg);

    m_labelTxt = new QLabel(this);
    m_labelTxt->setObjectName(QStringLiteral("label_txt"));
    m_labelTxt->setAlignment(Qt::AlignCenter);
    m_layout->addWidget(m_labelTxt);

    m_labelImg->setText(QString());
    m_labelTxt->setText(QString());
    // Реф. @0x682444: подпись скрыта, пока её не задали setText().
    m_labelTxt->setVisible(false);
}

void KImgListCell::clear()
{
    // Реф. @0x682680: видимость подписи НЕ трогается (только текст).
    m_labelImg->clear();
    m_labelTxt->setText(QString());
}

void KImgListCell::setPixmap(const QPixmap &pm)
{
    m_labelImg->setPixmap(pm);
}

void KImgListCell::setText(const QString &text)
{
    // Реф. @0x682740: текст И показ подписи.
    m_labelTxt->setText(text);
    m_labelTxt->setVisible(true);
}

void KImgListCell::setAlignment(Qt::Alignment a)
{
    m_labelImg->setAlignment(a);
    m_labelTxt->setAlignment(a);
}

void KImgListCell::setMaximumSize(int w, int h)
{
    // Реф. @0x682818: резерв под подпись — ТОЛЬКО когда она видима.
    if (m_labelTxt->isVisible())
        m_labelImg->setMaximumSize(w, h - kTextReserve);
    QFrame::setMaximumSize(w, h);
}

void KImgListCell::setMaximumSize(const QSize &s)
{
    setMaximumSize(s.width(), s.height());
}

void KImgListCell::setMinimumSize(int w, int h)
{
    // Реф. @0x6828f0.
    if (m_labelTxt->isVisible())
        m_labelImg->setMinimumSize(w, h - kTextReserve);
    QFrame::setMinimumSize(w, h);
}

void KImgListCell::setMinimumSize(const QSize &s)
{
    setMinimumSize(s.width(), s.height());
}
