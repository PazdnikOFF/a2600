#include "KMySlider.h"

#include <QLabel>
#include <QMouseEvent>
#include <QStyle>

KMySlider::KMySlider(Qt::Orientation orientation, QWidget *parent)
    : QSlider(orientation, parent)
{
    // Реф. ctor @0x78def0: дочерняя метка mySliderLabel + стили + SetLabelText.
    m_label = new QLabel(this);
    m_label->setObjectName(QStringLiteral("mySliderLabel"));
    m_label->setFixedSize(50, 20);
    m_label->setAutoFillBackground(true);
    m_label->setAlignment(Qt::AlignCenter);
    m_label->raise();
    m_label->move(0, -3);
    m_label->setStyleSheet(QStringLiteral(
        "QLabel#mySliderLabel { color: white; background: transparent; font-size: 10pt; }"));
    SetLabelText();
    // Порт: метка едет за ползунком + обновляет текст (реф. handle-follow латентна).
    connect(this, &QSlider::valueChanged, this, [this]() { SetLabelText(); SetLabelPosition(false); });
    SetLabelPosition(false);
}

void KMySlider::SetLabelText()
{
    // Реф. @0x78dd98: value, для положительных префикс «+».
    QString t = QString::number(value());
    if (value() > 0)
        t.prepend(QLatin1Char('+'));
    m_label->setText(t);
}

void KMySlider::SetLabelPosition(bool skip)
{
    // Реф. @0x78dce8: метка к ползунку (frac по value).
    if (skip)
        return;
    const int range = maximum() - minimum();
    if (range <= 0)
        return;
    const double frac = double(value() - minimum()) / range;
    const int travel = width() - m_label->width();
    m_label->move(int(frac * travel), 3);
}

void KMySlider::SetValue(int v)
{
    QSlider::setValue(v);
    SetLabelText();
}

void KMySlider::mousePressEvent(QMouseEvent *e)
{
    // Реф.: клик по дорожке → jump-to-click (value из позиции).
    if (e->button() == Qt::LeftButton) {
        const int range = maximum() - minimum();
        int v;
        if (orientation() == Qt::Horizontal)
            v = minimum() + range * e->pos().x() / qMax(1, width());
        else
            v = minimum() + range * (height() - e->pos().y()) / qMax(1, height());
        setValue(v);
    }
    QSlider::mousePressEvent(e);
}
