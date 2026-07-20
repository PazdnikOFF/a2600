#include "alg/KImageProcess.h"

#include <QBrush>
#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QRect>

#include <cmath>
#include <cstring>

#include "kernel/KSystemLog.h"
#include "ui/KDisplayOption.h"

namespace {

// Реф.: .rodata @0x891528 = 0x400921FF2E48E8A7 — это ЛИТЕРАЛЬНАЯ 3.1416,
// а НЕ M_PI. Отличие видно на выходе тональной кривой, поэтому сохранено.
const double kPi = 3.1416;

inline float max3(float a, float b, float c) { return std::max(a, std::max(b, c)); }
inline float min3(float a, float b, float c) { return std::min(a, std::min(b, c)); }

// Реф. кламп: v = x + 0.5f; u = (uint)v (fcvtzu — усечение, отрицательные → 0);
// результат = (v <= 255.0f) ? (uchar)(u & 0xFF) : 255.
inline uchar clampByte(float x)
{
    const float v = x + 0.5f;
    const unsigned u = (v <= 0.0f) ? 0u : unsigned(v);
    return (v <= 255.0f) ? uchar(u & 0xFFu) : uchar(255);
}

} // namespace

// --- цветовое пространство --------------------------------------------------

void KImageProcess::RGB2Ybcbr(const uchar *pRGB, float *pYCC, int width, int height)
{
    const int n = width * height;
    if (n <= 0)
        return;
    for (int i = 0; i < n; ++i) {
        const float R = pRGB[3 * i + 0];
        const float G = pRGB[3 * i + 1];
        const float B = pRGB[3 * i + 2];
        pYCC[3 * i + 0] =  0.2126f * R + 0.7152f * G + 0.0722f * B;
        pYCC[3 * i + 1] = -0.1146f * R - 0.3854f * G + 0.5f    * B;
        pYCC[3 * i + 2] =  0.5f    * R - 0.4542f * G - 0.0458f * B;
    }
}

void KImageProcess::Ybcbr2RGB(const float *pYCC, float *pRGB, int width, int height)
{
    const int n = width * height;
    if (n <= 0)
        return;
    for (int i = 0; i < n; ++i) {
        const float Y  = pYCC[3 * i + 0];
        const float Cb = pYCC[3 * i + 1];
        const float Cr = pYCC[3 * i + 2];
        // Умножения на 0.0f в реф. РЕАЛЬНО ВЫПОЛНЯЮТСЯ (movi+fmadd) — оставлены:
        // они превращают NaN/Inf в хроме в NaN, а не выбрасываются оптимизатором.
        pRGB[3 * i + 0] = Y + 0.0f    * Cb + 1.5748f * Cr;
        pRGB[3 * i + 1] = Y - 0.1873f * Cb - 0.4681f * Cr;
        pRGB[3 * i + 2] = Y + 1.8556f * Cb + 0.0f    * Cr;
    }
}

void KImageProcess::EnhanceBrightnessAndContrast(float *pYCC, int width, int height,
                                                 int nBrightness, int nContrast,
                                                 int nThreshold)
{
    const float t = float(std::tan(double(((nContrast / 255.0f) * 44.0f + 45.0f) / 180.0f) * kPi));
    const float k = nBrightness / 255.0f;
    const float lo = (1.0f - k) * -127.5f;
    const float hi = (1.0f + k) *  127.5f;
    const float A = hi + lo * t;
    const float thr = float(nThreshold);
    const float den = float(255 - nThreshold);
    const float a = ((255.0f - (A + thr * t)) / den - t) / den;
    const float b = t - 2.0f * a * thr;
    const float c = 255.0f - a * 65025.0f - 255.0f * b;   // 65025 == 255*255

    const int n = width * height;
    for (int i = 0; i < n; ++i) {
        const float Y = pYCC[3 * i + 0];
        if (thr >= Y) {
            // Линейная ветвь — чистый float.
            pYCC[3 * i + 0] = A + t * Y;
        } else {
            // Квадратичная ветвь — В DOUBLE, причём Y*b считается СНАЧАЛА во
            // float и лишь потом расширяется (важно для побитового совпадения).
            pYCC[3 * i + 0] = float(double(Y) * double(Y) * double(a)
                                    + double(Y * b) + double(c));
        }
    }
}

void KImageProcess::EnhanceSaturability(const float *pRGB, uchar *pOut,
                                        int width, int height, int nSat)
{
    const float sat = nSat / 100.0f;
    const int n = width * height;
    for (int i = 0; i < n; ++i) {
        const float R = pRGB[3 * i + 0];
        const float G = pRGB[3 * i + 1];
        const float B = pRGB[3 * i + 2];
        const float mx = max3(R, G, B);
        const float mn = min3(R, G, B);
        const float d = (mx - mn) / 255.0f;
        float Rn = R, Gn = G, Bn = B;
        if (d != 0.0f) {
            float sum = (mx + mn) / 255.0f;
            const float L = sum * 0.5f;
            if (L > 0.5f)
                sum = 2.0f - sum;
            const float S = d / sum;
            const float alpha = 1.0f / ((S + sat >= 1.0f) ? S : (1.0f - sat));
            const float Lv = L * 255.0f;
            const float m = alpha - 1.0f;
            Rn = R + (R - Lv) * m;
            Gn = G + (G - Lv) * m;
            Bn = B + (B - Lv) * m;
        }
        pOut[3 * i + 0] = clampByte(Rn);
        pOut[3 * i + 1] = clampByte(Gn);
        pOut[3 * i + 2] = clampByte(Bn);
    }
}

int KImageProcess::OptimizeReportImage(const QString &srcPath, QString dstPath)
{
    if (srcPath.isEmpty() || dstPath.isEmpty())
        return 5;
    QImage img(srcPath);
    if (img.isNull())
        return 5;

    const int w = img.width(), h = img.height();
    const int n = w * h;
    uchar *rgb  = new uchar[3 * n];   // плотный RGB (реф. вход RGB2Ybcbr)
    float *ycc  = new float[3 * n];
    float *rgb2 = new float[3 * n];
    uchar *out  = new uchar[3 * n];

    // Реф.: bits() трактуется как 32-битный ARGB/RGB32 (4 Б/пиксель), БЕЗ
    // convertToFormat и БЕЗ учёта bytesPerLine. Порядок в памяти — BGRA.
    uchar *bits = img.bits();
    for (int i = 0; i < n; ++i) {
        rgb[3 * i + 0] = bits[4 * i + 2];   // R
        rgb[3 * i + 1] = bits[4 * i + 1];   // G
        rgb[3 * i + 2] = bits[4 * i + 0];   // B
    }

    RGB2Ybcbr(rgb, ycc, w, h);
    // Производственный тюнинг реф.: 40 / 20 / 50 и насыщенность 20.
    EnhanceBrightnessAndContrast(ycc, w, h, 40, 20, 50);
    Ybcbr2RGB(ycc, rgb2, w, h);
    EnhanceSaturability(rgb2, out, w, h, 20);

    for (int i = 0; i < n; ++i) {
        bits[4 * i + 2] = out[3 * i + 0];
        bits[4 * i + 1] = out[3 * i + 1];
        bits[4 * i + 0] = out[3 * i + 2];
        // Альфа (bits[4*i+3]) не трогается — как в реф.
    }
    img.save(dstPath, nullptr, -1);   // КВИРК реф.: результат отбрасывается

    delete[] rgb;
    delete[] ycc;
    delete[] rgb2;
    delete[] out;
    return 1;
}

// --- геометрия --------------------------------------------------------------

void KImageProcess::GetGroupImageSize(const QImage &a, const QImage &b, _KGroupType type,
                                      _KGroupImgSize &out, int flag)
{
    // КВИРК реф.: при негодном A не пишется НИЧЕГО, даже imgB_*.
    if (a.height() <= 0 || a.width() <= 0)
        return;

    out.imgB_w = b.width();
    out.imgB_h = b.height();

    if (type == K_GROUP_B_LEFT_A_RIGHT) {
        const int H = 2 * b.height();
        const int thumbW = a.width() * H / a.height();
        out.totalW = thumbW + b.width();
        out.totalH = H;
        out.imgA_drawW = thumbW;
        out.imgA_drawH = H;
        out.B_x = 0;           out.B_y = H / 4;   // asr #2 — усекающее деление
        out.A_x = b.width();   out.A_y = 0;
    } else if (type == K_GROUP_A_LEFT_B_RIGHT) {
        if (flag == 1) {
            const int H = b.height();            // БЕЗ удвоения
            const int thumbW = a.width() * b.height() / a.height();
            out.totalW = thumbW + b.width();
            out.totalH = H;
            out.imgA_drawW = thumbW;
            out.imgA_drawH = H;
            out.A_x = 0;        out.A_y = 0;
            out.B_x = thumbW;   out.B_y = 0;     // без центрирования
        } else {
            const int H = 2 * b.height();
            const int thumbW = a.width() * H / a.height();
            out.totalW = thumbW + b.width();
            out.totalH = H;
            out.imgA_drawW = thumbW;
            out.imgA_drawH = H;
            out.A_x = 0;        out.A_y = 0;
            out.B_x = thumbW;   out.B_y = H / 4;
        }
    } else if (type >= K_GROUP_OVERLAY) {
        out.totalW = a.width();
        out.totalH = a.height();
        out.imgA_drawW = 0; out.imgA_drawH = 0;
        out.A_x = 0; out.A_y = 0; out.B_x = 0; out.B_y = 0;
    }
    // type 2 и 3: записаны только imgB_* ⇒ холст 0x0, ничего не рисуется.
}

QImage KImageProcess::CreateGroupImage(const QImage &a, const QImage &b, const _KPoint &pt,
                                       _KGroupType type, int flag)
{
    _KGroupImgSize sz;   // реф.: структура обнуляется целиком (0x28 байт)
    GetGroupImageSize(a, b, type, sz, flag);

    QImage img(sz.totalW, sz.totalH, QImage::Format_ARGB32);
    if (img.isNull())
        return img;
    QPainter p(&img);
    p.fillRect(img.rect(), QColor(Qt::white));

    if (type == K_GROUP_B_LEFT_A_RIGHT || type == K_GROUP_A_LEFT_B_RIGHT) {
        const QImage thumb = CreateThumbnail(a, sz.imgA_drawW, sz.imgA_drawH);
        // Реф. порядок: СНАЧАЛА B, потом миниатюра A.
        p.drawImage(QPointF(sz.B_x, sz.B_y), b);
        p.drawImage(QPointF(sz.A_x, sz.A_y), thumb);
    } else if (type >= K_GROUP_OVERLAY) {
        p.drawImage(QPointF(sz.A_x, sz.A_y), a);
        p.drawImage(QPointF(pt.x, pt.y), b);
    }
    // type 2/3 — ничего.
    p.end();
    return img;
}

QImage KImageProcess::CreateThumbnail(const QImage &img, int w, int h)
{
    // Реф.: оба режима переданы явно.
    return img.scaled(QSize(w, h), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

QSize KImageProcess::ScaledWithAspectRatio(const QSize &a, const QSize &b)
{
    const float ra = float(a.width()) / float(a.height());
    const float rb = float(b.width()) / float(b.height());
    // Реф.: fcmpe + b.le ⇒ равенство И NaN уходят в else (ограничение по ширине).
    if (rb > ra)
        return QSize(a.width() * b.height() / a.height(), b.height());
    return QSize(b.width(), b.width() * a.height() / a.width());
}

int KImageProcess::ResizeCopyImage(QString srcPath, QString dstPath, _LoadImgType type)
{
    if (srcPath.isEmpty() || dstPath.isEmpty())
        return 5;
    if (dstPath == srcPath)
        return 1;                       // no-op

    int targetW, targetH;
    if (type == K_LOAD_IMG_150x30)      { targetW = 150; targetH = 30;  }
    else if (type == K_LOAD_IMG_850x120){ targetW = 850; targetH = 120; }
    else                                { targetW = 160; targetH = 120; }

    if (QFile::exists(dstPath))
        QDir().remove(dstPath);
    QImage src(srcPath);
    if (src.isNull())
        return 0;

    const int srcW = src.width(), srcH = src.height();
    int newW = srcW, newH = srcH;
    if (targetW != srcW || targetH != srcH) {
        // Реф.: вписывание считается ВРУЧНУЮ во float32 с усечением (fcvtzs).
        const float ar = float(srcW) / float(srcH);
        newW = targetW;
        newH = int(targetW / ar);
        if (targetH < newH) {
            newH = targetH;
            newW = int(targetH * ar);
        }
    }
    // Реф.: здесь FastTransformation (в CreateThumbnail — Smooth).
    const QImage s = src.scaled(QSize(newW, newH), Qt::IgnoreAspectRatio,
                                Qt::FastTransformation);
    if (targetW <= newW && targetH <= newH)
        return s.save(dstPath, nullptr, -1) ? 1 : 2;

    // Иначе — подложка (letterbox) белым.
    QImage canvas(targetW, targetH, QImage::Format_RGB888);
    QPainter p(&canvas);
    p.setPen(QColor(255, 255, 255, 255));
    p.setBrush(QBrush(QColor(255, 255, 255, 255), Qt::SolidPattern));
    // Реф.: QRect строится из СЫРЫХ координат {0,0,targetW-1,targetH-1}.
    QRect r(0, 0, targetW - 1, targetH - 1);
    p.drawRects(&r, 1);
    p.drawImage(QPointF((targetW - newW) / 2, (targetH - newH) / 2), s);
    p.end();
    return canvas.save(dstPath, nullptr, -1) ? 1 : 2;
}

// --- пути и иконки видео ----------------------------------------------------

QString KImageProcess::GetVideoImg(const QString &path)
{
    const QFileInfo fi(path);
    return fi.absolutePath() + "/" + fi.baseName() + ".jpg";
}

void KImageProcess::FormatScreenShotImg(const char *path)
{
    if (!path)
        return;
    QImage src(QString::fromUtf8(path));
    if (src.isNull())
        return;

    // Реф.: обнулённый стековый буфер 190*70*3 == 39900 байт, bytesPerLine 570.
    const int ow = 190, oh = 70, stride = 570;
    QByteArray buf(stride * oh, '\0');
    QImage overlay(reinterpret_cast<uchar *>(buf.data()), ow, oh, stride,
                   QImage::Format_RGB888);

    // Реф.: точка берётся из структуры KDisplayOption::GetSoftEndoViewConf()
    // по смещениям +0x30/+0x34. У нас GetSoftEndoViewConf возвращает ПУТЬ к ini
    // (иная сигнатура) ⇒ берём именованный прямоугольник раскладки рабочего
    // стола. Соответствие смещений полям НЕ УСТАНОВЛЕНО (см. PROGRESS §10).
    _KPoint pt;
    const QMap<QString, QRect> conf = KDisplayOption::Instance().GetDesktopViewConf(false);
    if (!conf.isEmpty()) {
        const QRect r = conf.first();
        pt.x = r.x();
        pt.y = r.y();
    }

    const QImage merged = CreateGroupImage(src, overlay, pt, K_GROUP_OVERLAY, 0);
    merged.save(QString::fromUtf8(path), nullptr, -1);   // ПОВЕРХ ТОГО ЖЕ ФАЙЛА
}

bool KImageProcess::MergeVideoSmallImage(const QString &path)
{
    QString iconPath = KDisplayOption::GetThemeQssPath(QStringLiteral("icon/video/"));
    iconPath.append(QStringLiteral("center.png"));
    // Реф.: иконка проверяется ПЕРВОЙ.
    if (!QFile(iconPath).exists() || !QFile(path).exists()) {
        LogPrintfEx(true, "[APP][E]: ", "[KVideoIcon::mergeSmallImage]File missing.\n");
        return false;
    }
    const QImage base(path);
    const QImage icon(iconPath);
    const int dw = base.width() - icon.width();
    const int dh = base.height() - icon.height();
    if (!(dw > 1 && dh > 1))
        return false;                              // реф.: без лога

    QImage out(base.width(), base.height(), QImage::Format_ARGB32);
    QPainter p(&out);
    p.drawImage(QPointF(0, 0), base);
    p.drawImage(QPointF(dw >> 1, dh >> 1), icon);  // по центру
    p.end();
    const QFileInfo fi(path);
    return out.save(fi.absolutePath() + "/" + fi.baseName() + "_s.jpg", nullptr, -1);
}

bool KImageProcess::MergeVideoBigImage(const QString &path)
{
    QString iconPath = KDisplayOption::GetThemeQssPath(QStringLiteral("icon/video/"));
    iconPath.append(QStringLiteral("play.png"));
    if (!QFile(iconPath).exists() || !QFile(path).exists()) {
        LogPrintfEx(true, "[APP][E]: ", "[VideoIcon::mergeBigIcon]File missing.\n");
        return false;
    }
    const QImage base(path);
    const QImage icon(iconPath);
    // КВИРК реф.: ширина НЕ проверяется вообще.
    const int y = base.height() - (icon.height() + 100);
    if (y <= 0) {
        LogPrintfEx(true, "[APP][E]: ", "mergeBigIcon: image size error\n");
        return false;
    }

    QImage out(base.width(), base.height(), QImage::Format_ARGB32);
    QPainter p(&out);
    p.drawImage(QPointF(0, 0), base);
    p.drawImage(QPointF(100.0, double(y)), icon);   // 100.0 — литеральный double
    p.end();
    const QFileInfo fi(path);
    const bool ok = out.save(fi.absolutePath() + "/res/" + fi.baseName() + "_v.jpg",
                             nullptr, -1);
    if (!ok)
        LogPrintfEx(true, "[APP][E]: ", "Save video big icon failed\n");
    return ok;
}
