#pragma once
// СГЕНЕРИРОВАНО tools/gen_camera.py — НЕ РЕДАКТИРОВАТЬ.
// Источник: KCamera::InitCameraSeriesMap() @0x6b3dc8 в update/root/X2000.

#include <QMap>
#include <QString>

// Модель камеры -> код серии. Отдаётся через KCamera::GetCameraSeriesMap().
static const QMap<QString, QString> kCameraSeriesMap = {
    { QStringLiteral("10-110-201"), QStringLiteral("2I2") },
    { QStringLiteral("10-111-201"), QStringLiteral("2I3") },
    { QStringLiteral("10-112-201"), QStringLiteral("2I4") },
    { QStringLiteral("10-100-201"), QStringLiteral("4I6") },
    { QStringLiteral("10-110-201V"), QStringLiteral("8I7") },
    { QStringLiteral("10-111-201V"), QStringLiteral("8I8") },
    { QStringLiteral("10-112-201V"), QStringLiteral("8I9") },
};
