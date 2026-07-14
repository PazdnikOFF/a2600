#include "endo/KSoftEndoParam.h"
#include "ui/KDisplayOption.h"

#include <QSettings>
#include <QRect>

KSoftEndoParam::KSoftEndoParam()
{
    viewConf_ = KDisplayOption::Instance().GetSoftEndoViewConf();
}

QString KSoftEndoParam::HexKey(const QString &scopeName)
{
    // Секции video.ini — hex-кодировка ASCII имени модели ("EC-X20" → 45432d583230)
    QString hex;
    const QByteArray b = scopeName.toLatin1();
    for (char c : b)
        hex += QString("%1").arg(static_cast<unsigned char>(c), 2, 16, QChar('0'));
    return hex;
}

QSize KSoftEndoParam::GetVideoSize(const QString &scopeName) const
{
    QSettings ini(viewConf_, QSettings::IniFormat);
    const QRect r = ini.value(HexKey(scopeName) + "/videoSize").toRect();
    return r.size();
}

QString KSoftEndoParam::GetSensorType(const QString &scopeName) const
{
    QSettings ini(viewConf_, QSettings::IniFormat);
    return ini.value(HexKey(scopeName) + "/sensorType").toString();
}

QString KSoftEndoParam::GetEndoType(const QString &scopeName) const
{
    QSettings ini(viewConf_, QSettings::IniFormat);
    return ini.value(HexKey(scopeName) + "/endoType").toString();
}

QString KSoftEndoParam::GetFirmwareType(const QString &scopeName) const
{
    QSettings ini(viewConf_, QSettings::IniFormat);
    return ini.value(HexKey(scopeName) + "/firmwareType").toString();
}

bool KSoftEndoParam::IsDefaultMatch(const QString &scopeName) const
{
    QSettings ini(viewConf_, QSettings::IniFormat);
    return ini.value(HexKey(scopeName) + "/defaultMatch").toBool();
}
