#include "KVideoLabel.h"
#include "KDisplayOption.h"

#include <QImage>
#include <QPainter>
#include <QPaintEvent>

#include <cmath>

namespace {
// Реф. перо одинаково во всех рисующих функциях: ширина 1.0, SolidLine, SquareCap, BevelJoin.
QPen RefPen(Qt::GlobalColor c)
{
    return QPen(QBrush(c, Qt::SolidPattern), 1.0, Qt::SolidLine, Qt::SquareCap, Qt::BevelJoin);
}

// Реф. делит на π (сырые байты 0x400921FB54442D18 @0x88b050) при переводе рад→град.
double Deg(double rad)
{
    return rad * 180.0 / M_PI;
}

// Крестик ±10 (реф. DrawFlag рисует именно такие маркеры).
void Cross(QPainter &p, const QPointF &c)
{
    p.drawLine(QLineF(c.x() - 10.0, c.y(), c.x() + 10.0, c.y()));
    p.drawLine(QLineF(c.x(), c.y() - 10.0, c.x(), c.y() + 10.0));
}
}   // namespace

KVideoLabel::KVideoLabel(QWidget *parent)
    : QLabel(parent)
{
    // Реф. ctor @0x6876e0.
    setAutoFillBackground(true);
    setWindowOpacity(1.0);
    setAttribute(Qt::WA_TranslucentBackground, true);   // реф. атрибут 120
    m_videoRect = QRect();                              // невалидный (x2<x1)
}

void KVideoLabel::setVideoRect(QRect r)
{
    m_videoRect = r;   // реф. @0x687bf8
}

void KVideoLabel::setImageView(QString path)
{
    m_imagePath = std::move(path);   // реф. @0x687c78
}

void KVideoLabel::stopImageView()
{
    m_imagePath.clear();             // реф. @0x687cd8
}

bool KVideoLabel::isImageView() const
{
    return !m_imagePath.isEmpty();   // реф. @0x687d60 — проверка size у QStringData
}

void KVideoLabel::DrawLine(int mode, double d)
{
    // Реф. @0x687d78 — ровно три действия.
    m_mode = mode;
    m_t = d * 0.5 + 0.5;
    update();
}

void KVideoLabel::SetCornerCuttingProvider(std::function<bool(int &, QVector<QLine> &)> fn)
{
    m_cornerCutting = std::move(fn);
}

void KVideoLabel::PaintVideoLabel(QPainter &p, QRect rect)
{
    // Реф. @0x687ae0: СИНЕЕ перо (Qt::GlobalColor 9) и рамка четырьмя отдельными линиями.
    // ⚠️ В paintEvent эта функция НЕ вызывается — вызывающий код в реф. не найден.
    p.setPen(RefPen(Qt::blue));
    const int w = rect.width();
    const int h = rect.height();
    p.drawLine(QLine(0, 0, w, 0));
    p.drawLine(QLine(0, 0, 0, h));
    p.drawLine(QLine(w, h, 0, h));
    p.drawLine(QLine(w, h, w, 0));
}

QVector<QPair<int, int>> KVideoLabel::ArcSpans(const QRect &rect, int d)
{
    // Реф. @0x6877d8. Углы Qt — в 1/16 градуса.
    QVector<QPair<int, int>> out;
    const int w = rect.width();
    const int h = rect.height();
    if (d <= 0)
        return out;

    if (w == h) {
        out.append(qMakePair(0, 5760));            // 5760 = 360° × 16 — полный круг
        return out;
    }
    if (w > h) {
        const double a = Deg(std::acos((h / 2.0) / d));
        const int span = int((180.0 - 2.0 * a) * 16);
        out.append(qMakePair(int((a + 90.0) * 16), span));
        out.append(qMakePair(int((a - 90.0) * 16), span));
        return out;
    }
    const double a = Deg(std::acos((w / 2.0) / d));
    const int span = int((180.0 - 2.0 * a) * 16);
    out.append(qMakePair(int(a * 16), span));
    out.append(qMakePair(int((a + 180.0) * 16), span));
    return out;
}

void KVideoLabel::PaintDoubleArc(QPainter &p, QRect rect, int d)
{
    // Реф. @0x6877d8: круг радиуса d с центром в (w/2, h/2).
    p.setPen(RefPen(Qt::red));
    const QRectF circle(rect.width() / 2.0 - d, rect.height() / 2.0 - d, 2.0 * d, 2.0 * d);
    for (const auto &s : ArcSpans(rect, d))
        p.drawArc(circle, s.first, s.second);
}

QVector<QLineF> KVideoLabel::DoubleLines(const QRect &rect, int d)
{
    // Реф. @0x6880b0.
    QVector<QLineF> out;
    const int w = rect.width();
    const int h = rect.height();
    if (w == h)
        return out;                     // реф. выходит сразу

    // Под корнем именно d² − (половина стороны)², а НЕ наоборот: в реф. это
    // `fnmsub d0, d1, d1, d0` @0x688188, где d1 = d (радиус), а d0 = (сторона/2)²
    // (деление на 2 видно по `add w21,w21,w21,lsr #31; asr w21,#1` @0x688170).
    // Геометрически это та же величина: круг радиуса d с центром (w/2, h/2) пересекает
    // боковую грань при d > w/2, и половина хорды = sqrt(d² − (w/2)²).
    if (w < h) {                        // высокий: две ВЕРТИКАЛЬНЫЕ линии по бокам
        const double half = w / 2.0;
        const double s = double(d) * d - half * half;
        if (s < 0)
            return out;                 // круг не достаёт до граней — линий нет
        const double h8 = std::sqrt(s);
        const double cy = h / 2.0;
        out.append(QLineF(0, cy - h8, 0, cy + h8));
        out.append(QLineF(w - 1, cy - h8, w - 1, cy + h8));
        return out;
    }
    // широкий: зеркально — две ГОРИЗОНТАЛЬНЫЕ линии сверху и снизу
    const double half = h / 2.0;
    const double s = double(d) * d - half * half;
    if (s < 0)
        return out;
    const double w8 = std::sqrt(s);
    const double cx = w / 2.0;
    out.append(QLineF(cx - w8, 0, cx + w8, 0));
    out.append(QLineF(cx - w8, h - 1, cx + w8, h - 1));
    return out;
}

void KVideoLabel::PaintDoubleLine(QPainter &p, QRect rect, int d)
{
    p.setPen(RefPen(Qt::red));
    const QVector<QLineF> lines = DoubleLines(rect, d);
    for (const QLineF &l : lines)
        p.drawLine(l);
    if (lines.isEmpty())
        return;
    // Реф.: после линий, если t != 0, рисует флаги в прямоугольнике между сторонами.
    if (m_t != 0.0) {
        QRect r(int(lines[0].x1()), int(lines[0].y1()),
                int(lines[1].x2() - lines[0].x1()), int(lines[1].y2() - lines[0].y1()));
        DrawFlag(p, r);
    }
}

void KVideoLabel::DrawFlag(QPainter &p, QRect &r)
{
    // Реф. @0x687d90: ПЯТЬ крестиков ±10, СВОЁ перо не задаёт — берёт текущее.
    // D — «сырой» аргумент DrawLine, восстановленный из t: D = 2t - 1.
    const double t = m_t;
    const double D = 2.0 * t - 1.0;

    // (A) целочисленный крестик в центре ВНЕШНЕГО прямоугольника (реф. поле +0x28).
    if (m_markerRect.isValid()) {
        const QPoint c = m_markerRect.center();
        p.drawLine(QLine(c.x() - 10, c.y(), c.x() + 10, c.y()));
        p.drawLine(QLine(c.x(), c.y() - 10, c.x(), c.y() + 10));
    }

    const QPointF tl = r.topLeft();
    const QPointF br = r.bottomRight();
    auto lerp = [&](double k) {
        return QPointF(tl.x() + (br.x() - tl.x()) * k, tl.y() + (br.y() - tl.y()) * k);
    };
    const QPointF P = lerp(1.0 - t);   // (B)
    Cross(p, P);
    Cross(p, QPointF(P.x() + r.width() * D, P.y()));    // (C)
    Cross(p, QPointF(P.x(), P.y() + r.height() * D));   // (D)
    Cross(p, lerp(t));                                   // (E)
}

void KVideoLabel::paintEvent(QPaintEvent *)
{
    // Реф. @0x688328.
    if (!m_videoRect.isValid())        // реф. guard: x2 < x1 → выход без рисования
        return;

    QPainter p(this);

    if (isImageView()) {
        // Реф.: картинка центрируется во ВНЕШНЕМ прямоугольнике (+0x28), НЕ в m_videoRect.
        const QImage img(m_imagePath);
        const QRect host = m_markerRect.isValid() ? m_markerRect : m_videoRect;
        if (!img.isNull())
            p.drawImage(QPointF(host.center().x() - img.width() / 2.0,
                                host.center().y() - img.height() / 2.0), img);
    } else {
        p.eraseRect(m_videoRect);
    }

    if (m_mode != 0) {
        p.setPen(RefPen(Qt::red));
        const QRect imgPro = KDisplayOption::Instance().getVideoRectForImgPro();
        int radius = 0;
        QVector<QLine> lines;
        const bool haveInfo = m_cornerCutting ? m_cornerCutting(radius, lines) : false;

        if (m_mode == 1 && haveInfo) {
            // Реф.: 8 прямых контура «среза углов» — геометрия целиком из device-данных.
            for (const QLine &l : lines)
                p.drawLine(l);
            if (m_t != 0.0) {
                QRect r = imgPro;
                DrawFlag(p, r);
            }
        } else if (m_mode == 2 && haveInfo) {
            PaintDoubleArc(p, imgPro, radius);
            PaintDoubleLine(p, imgPro, radius);
        }
    }

    if (isImageView())
        return;
    if (m_mode == 0) {
        // Реф.: ставит CompositionMode_Clear перед разрушением painter'а и больше
        // ничего не рисует — эффект нулевой. Воспроизведено как есть.
        p.setCompositionMode(QPainter::CompositionMode_Clear);
    }
}
