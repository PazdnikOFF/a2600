#pragma once

#include <QString>
#include <QSize>

// Параметры вида гибкого эндоскопа (реф. класс KSoftEndoParam, X-2600).
// Читает scope/video.ini (KDisplayOption::GetSoftEndoViewConf) — секции ini
// это hex-имена моделей (напр. "EC-X20" → "45432d583230"). Отдаёт videoSize,
// sensorType, endoType и т.д. videoSize определяет выбор IMG-раскладки.
class KSoftEndoParam
{
public:
    KSoftEndoParam();

    // По имени модели ("EC-X20") — hex-ключ секции ini.
    static QString HexKey(const QString &scopeName);

    QSize   GetVideoSize(const QString &scopeName) const;   // [key]/videoSize → размер
    QString GetSensorType(const QString &scopeName) const;  // OV2740/OH01A/IMX274…
    QString GetEndoType(const QString &scopeName) const;
    QString GetFirmwareType(const QString &scopeName) const;
    bool    IsDefaultMatch(const QString &scopeName) const;

private:
    QString viewConf_;   // путь к video.ini
};
