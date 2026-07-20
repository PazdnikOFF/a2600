#pragma once

#include <QSize>
#include <QSizeF>

// Менеджер экрана (реф. KScreenMng, X-2600). Синглтон (call_once, куча, НЕ освобождается).
// sizeof 0x20. Хранит основное разрешение и множители пересчёта под опорные 1920×1080
// («1K» — вендорское имя опорной ШИРИНЫ). ЧИСТЫЙ Qt (QGuiApplication::primaryScreen),
// НЕ железо: под ui_preview/offscreen честно читает размер offscreen-экрана. Используется
// KRTTextItemCreator для масштабирования шрифта отчёта под фактический экран.
//
// Раскладка реф. (адреса геттеров — тривиальные загрузки члена):
//   +0x00 QSize  m_mainResolution   (реф. @0x4b9cf8 GetMainResolution: ldr x0,[x0])
//   +0x08 double m_widthRatio        }
//   +0x10 double m_heightRatio       } реф. @0x4b9d08 GetSizeFRatioTo1K: ldp d0,d1,[x0,#8]
//   +0x18 double m_ratio (== m_widthRatio; реф. @0x4b9d00 GetRatioTo1K: ldr d0,[x0,#24])
// Формулы ctor (@0x4b9d10): widthRatio=W/1920.0, heightRatio=H/1080.0; каждый КЛАМП к 1.0
// ТОЛЬКО если ≤0 (реф. fcsel hi). m_ratio = widthRatio. При 1920×1080 ratio ровно 1.0.
class KScreenMng
{
public:
    static KScreenMng *GetInstance();

    QSize  GetMainResolution() const { return m_mainResolution; }   // реф. @0x4b9cf8
    double GetRatioTo1K() const { return m_ratio; }                 // реф. @0x4b9d00 (=widthRatio)
    QSizeF GetSizeFRatioTo1K() const                                // реф. @0x4b9d08
    {
        return QSizeF(m_widthRatio, m_heightRatio);
    }

private:
    KScreenMng();                                                   // реф. @0x4b9d10

    QSize  m_mainResolution;   // +0x00
    double m_widthRatio;       // +0x08
    double m_heightRatio;      // +0x10
    double m_ratio;            // +0x18
};
