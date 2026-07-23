#pragma once

#include <QLabel>
#include <QLine>
#include <QRect>
#include <QString>
#include <QVector>

#include <functional>

// Оверлей поверх области видео (реф. KVideoLabel : QLabel, ctor @0x6876e0,
// файл прошивки kvideolabel.cpp). Чисто рисующий виджет — портируется целиком.
// Владелец: `KViewSoftEndo::UpdateVideoLabel(QRect)` @0x467368 рассылает геометрию
// видео-области ему, `KFloatingMsg` и `KMsgPopup`.
//
// Реф. ctor: `setAutoFillBackground(true)`, `setWindowOpacity(1.0)`,
// `setAttribute((Qt::WidgetAttribute)120, true)` — 120 это `Qt::WA_TranslucentBackground`;
// поля: m_videoRect (+0x30) = невалидный QRect, m_imagePath (+0x40) = "", режим (+0x48) = 0,
// параметр t (+0x50) = 0.0. `setRenderHint(Antialiasing)` НЕ включается НИГДЕ.
//
// Перо во всех рисующих функциях одинаковое: `QPen(QBrush(цвет, Qt::SolidPattern), 1.0,
// Qt::SolidLine, Qt::SquareCap, Qt::BevelJoin)`. Цвета — сырые Qt::GlobalColor из дизасма:
// 7 = Qt::red (дуги/двойные линии/флаги), 9 = Qt::blue (PaintVideoLabel).
//
// DEVICE-STUB: реф. paintEvent берёт прямоугольник из `KDisplayOption::getVideoRectForImgPro()`
// (у нас есть) и данные среза углов из `KVideoSet::GetCornerCuttingInfo()` (device) —
// последнее вынесено за инъектируемый провайдер.
class KVideoLabel : public QLabel
{
    Q_OBJECT
public:
    explicit KVideoLabel(QWidget *parent = nullptr);

    void setVideoRect(QRect r);              // реф. @0x687bf8 → +0x30
    void setImageView(QString path);         // реф. @0x687c78 → +0x40
    void stopImageView();                    // реф. @0x687cd8 — очищает путь
    bool isImageView() const;                // реф. @0x687d60 — !m_imagePath.isEmpty()

    // Реф. @0x687d78 целиком: `+0x48 = mode; +0x50 = d*0.5 + 0.5; update();`
    // Второй аргумент приходит «сырым» (D ∈ [-1,1]) и хранится уже как t ∈ [0,1].
    void DrawLine(int mode, double d);

    int    Mode() const { return m_mode; }   // +0x48
    double T() const { return m_t; }         // +0x50

    // Реф. поле +0x28 — указатель на ВНЕШНИЙ прямоугольник (опора маркера A в DrawFlag
    // и центр картинки в image-view). Его СЕТТЕР в бинарнике НЕ ВОССТАНОВЛЕН (ни ctor,
    // ни один из 11 методов класса туда не пишет) — здесь заведён явный сеттер.
    void SetMarkerRect(QRect r) { m_markerRect = r; }
    QRect MarkerRect() const { return m_markerRect; }

    // DEVICE-STUB: реф. KVideoSet::GetCornerCuttingInfo(). Для режима 1 отдаёт линии
    // контура среза углов, для режима 2 — радиус скругления.
    void SetCornerCuttingProvider(std::function<bool(int &radius, QVector<QLine> &lines)> fn);

    // ── Рисующие функции (реф. имена). Публичны, чтобы порт можно было проверить
    //    покадрово; в реф. это обычные члены. ──
    void PaintVideoLabel(QPainter &p, QRect rect);            // реф. @0x687ae0
    void PaintDoubleArc(QPainter &p, QRect rect, int d);      // реф. @0x6877d8
    void PaintDoubleLine(QPainter &p, QRect rect, int d);     // реф. @0x6880b0
    void DrawFlag(QPainter &p, QRect &r);                     // реф. @0x687d90

    // ── Геометрия, вынесенная из рисующих функций, чтобы быть проверяемой ──
    // (в реф. считается прямо внутри; вынос — наш, поведение идентично).
    // Пары (startAngle, spanAngle) в 1/16 градуса, как требует QPainter::drawArc.
    static QVector<QPair<int, int>> ArcSpans(const QRect &rect, int d);
    // Две линии «капсулы» по бокам (высокий прямоугольник) или сверху/снизу (широкий).
    static QVector<QLineF> DoubleLines(const QRect &rect, int d);

protected:
    void paintEvent(QPaintEvent *e) override;   // реф. @0x688328

private:
    QRect   m_videoRect;                  // +0x30 (по умолчанию НЕВАЛИДНЫЙ)
    QString m_imagePath;                  // +0x40
    int     m_mode = 0;                   // +0x48 (0=выкл, 1=срез углов, 2=капсула)
    double  m_t = 0.0;                    // +0x50
    QRect   m_markerRect;                 // +0x28 (внешний, сеттер реф. не восстановлен)
    std::function<bool(int &, QVector<QLine> &)> m_cornerCutting;
};
