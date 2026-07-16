#include "sys/KSystem.h"
#include "ui/Theme.h"

#include <QGuiApplication>
#include <QScreen>
#include <QDir>
#include <QSettings>

namespace KSystem {

QString AppPath()            { return theme::root(); }
QString SystemPath()         { return QDir(theme::root()).absoluteFilePath("system"); }
QString DataPath()           { return QDir(theme::root()).absoluteFilePath("data"); }
QString DisplayConfigPath()  { return QDir(SystemPath()).absoluteFilePath("display"); }
QString UserPresetPath()     { return QDir(SystemPath()).absoluteFilePath("presetdata/userpreset"); }
QString VideoConfPath()      { return QDir(SystemPath()).absoluteFilePath("videoconf"); }
// Реф.: SetDataPath = DataPath + "setdata/", UserSetPath = SetDataPath + "userset/".
QString SetDataPath()        { return QDir(DataPath()).absoluteFilePath("setdata"); }
QString UserSetPath()        { return QDir(SetDataPath()).absoluteFilePath("userset"); }
QString ProtectedPath()      { return QDir(DataPath()).absoluteFilePath("protected"); }

QString ProductDisplayConfigPath(const QString &model)
{
    return QDir(DisplayConfigPath()).absoluteFilePath(model + "/" + model);
}

QString ProductDisplayConfigFile(const QString &model)
{
    return QDir(ProductDisplayConfigPath(model)).absoluteFilePath("product.ini");
}

QSize GetSystemResolution()
{
    if (QScreen *s = QGuiApplication::primaryScreen())
        return s->geometry().size();
    return QSize(1920, 1080);
}

QString ProductModel()
{
    // Оригинал берёт модель из display/project.ini ([Option] ProjectName).
    QSettings ini(QDir(DisplayConfigPath()).absoluteFilePath("project.ini"),
                  QSettings::IniFormat);
    const QString name = ini.value("Option/ProjectName").toString();
    return name.isEmpty() ? QStringLiteral("X-2600") : name;
}

} // namespace KSystem
