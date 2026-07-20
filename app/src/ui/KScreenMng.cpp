#include "KScreenMng.h"

#include <QGuiApplication>
#include <QScreen>

#include <mutex>

KScreenMng::KScreenMng()
    : m_mainResolution(1920, 1080)   // реф. дефолт: 0x780(1920) | 0x438(1080)<<32
    , m_widthRatio(-1.0)             // реф. init -1.0 (stp d0,d0,[+8] с d0=-1.0)
    , m_heightRatio(-1.0)
    , m_ratio(-1.0)
{
    // Реф.: primaryScreen() != null → перезаписать разрешение реальным размером экрана.
    if (QScreen *s = QGuiApplication::primaryScreen())
        m_mainResolution = s->size();

    double wr = m_mainResolution.width() / 1920.0;    // реф. d3 = 1920.0
    double hr = m_mainResolution.height() / 1080.0;   // реф. d2 = 1080.0
    // Реф. fcsel hi: оставить значение, ТОЛЬКО если > 0, иначе 1.0.
    if (!(wr > 0.0)) wr = 1.0;
    if (!(hr > 0.0)) hr = 1.0;

    m_ratio       = wr;   // реф. str d0,[+0x18] — m_ratio = widthRatio
    m_widthRatio  = wr;   // +0x08
    m_heightRatio = hr;   // +0x10
}

KScreenMng *KScreenMng::GetInstance()
{
    // Реф.: call_once + heap-аллокация 0x20; объект не освобождается (утечка by design).
    static std::once_flag flag;
    static KScreenMng *inst = nullptr;
    std::call_once(flag, [] { inst = new KScreenMng(); });
    return inst;
}
