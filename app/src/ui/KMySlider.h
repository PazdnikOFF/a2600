#pragma once

#include <QSlider>

class QLabel;

// Слайдер с плавающей меткой значения (реф. KMySlider @ctor 0x78def0, base QSlider). UI-порт.
// Дочерняя QLabel «mySliderLabel» (50×20, белый текст, transparent, 10pt, center) показывает
// value (с «+» для положительных). Клик по дорожке → jump-to-click. Реф.: метка позиционируется
// раз в ctor, обновляется только текст (handle-follow-ветка латентна) — в порте связываем
// valueChanged→трек метки к ползунку (полезнее). 100% PORT.
class KMySlider : public QSlider
{
    Q_OBJECT
public:
    explicit KMySlider(Qt::Orientation orientation = Qt::Horizontal, QWidget *parent = nullptr);

    void SetValue(int v);              // реф. @0x78dec8
    void SetLabelText();               // реф. @0x78dd98: value с «+»
    void SetLabelPosition(bool skip);  // реф. @0x78dce8: метка к ползунку

protected:
    void mousePressEvent(QMouseEvent *) override;   // jump-to-click

private:
    QLabel *m_label = nullptr;   // +0x30
};
