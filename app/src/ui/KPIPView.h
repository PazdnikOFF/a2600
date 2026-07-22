#pragma once

#include <QLabel>
#include <QObject>
#include <QRect>

// Оверлей-маска окна «картинка-в-картинке» (реф. KPIPViewRect : QLabel, ctor @0x70b1c8,
// size 0x50). UI-порт. НЕ цветная рамка, а КОМПОЗИТ-ВЫРЕЗ: paintEvent композит-режимом Clear
// «пробивает» (скруглённую) прозрачную дыру во весь виджет — виджет заливается фоном палитры/
// QSS (autoFillBackground), а Clear стирает скруглённую область → остаются только скруглённые
// углы поверх видео. Геометрию задаёт вызывающий (setRect/refresh → move/resize). БЕЗ device.
class KPIPViewRect : public QLabel
{
    Q_OBJECT
public:
    explicit KPIPViewRect(QWidget *parent = nullptr);

    void setRect(const QRect &r) { m_rect = r; }         // реф. хранит m_rect (+0x30)
    void setRadius(const QRect &r) { m_radiusRect = r; } // реф. хранит m_radiusRect (+0x40), юзаются .x()/.y()
    void refresh();                                       // реф.: move+resize к m_rect + repaint
    void setVisible(bool v) override;                     // реф.: если v → refresh, затем база

protected:
    void paintEvent(QPaintEvent *) override;   // реф.: CompositionMode_Clear + fill/roundRect

private:
    QRect m_rect{0, 0, -1, -1};        // +0x30
    QRect m_radiusRect{0, 0, -1, -1};  // +0x40
};

// Тонкая обёртка (реф. KPIPView : QObject, ctor @..., size 0x30). Держит один KPIPViewRect
// (+0x10) + геометрию (+0x20). Реф. имеет 2-й слот rect (+0x18) — присваивается ВНЕШНЕ
// (dual-view), в бинарнике источник не найден → в порте одиночный rect (верно для дефолта).
class KPIPView : public QObject
{
    Q_OBJECT
public:
    explicit KPIPView(QWidget *parent = nullptr);

    void refresh(const QRect &r);           // реф.: если r валиден — store; rect->refresh
    void refresh() { refresh(QRect(0, 0, -1, -1)); }
    void setRadius(const QRect &r);         // реф.: forward → rect->setRadius
    void setVisible(bool v);                // реф.: если v → refresh; затем rect->setVisible

    KPIPViewRect *rect() const { return m_rect; }

private:
    KPIPViewRect *m_rect = nullptr;   // +0x10
    QRect m_geom{0, 0, -1, -1};       // +0x20
};
