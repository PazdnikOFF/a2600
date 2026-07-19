#include "sys/KSystem.h"
#include "ui/Theme.h"

#include <QGuiApplication>
#include <QScreen>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>

namespace KSystem {

// Реф.: RootPath — литерал "/home/root/", остальное — конкатенация от него.
// У нас корень подменяется через ENDO_ROOT (theme::root()) для отладки на Mac.
QString RootPath()           { return theme::root(); }
QString SystemPath()         { return QDir(RootPath()).absoluteFilePath("system"); }
QString DataPath()           { return QDir(RootPath()).absoluteFilePath("data"); }
QString AppPath()            { return QDir(DataPath()).absoluteFilePath("app"); }
QString DisplayConfigPath()  { return QDir(SystemPath()).absoluteFilePath("display"); }
QString SystemPresetPath()   { return QDir(SystemPath()).absoluteFilePath("presetdata"); }
QString ProjectPresetPath()  { return QDir(SystemPresetPath()).absoluteFilePath("syspreset"); }
QString ProjectUserPresetPath() { return QDir(SystemPresetPath()).absoluteFilePath("userpreset"); }
QString UserPresetPath()     { return ProjectUserPresetPath(); }
QString VideoConfPath()      { return QDir(SystemPath()).absoluteFilePath("videoconf"); }
// Реф. KSystem::ColdlightConfigPath @0x673e80 = SystemPath() + "coldlight/".
QString ColdlightConfigPath() { return QDir(SystemPath()).absoluteFilePath("coldlight"); }
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

bool CopyDirectoryFiles(const QString &src, const QString &dst, bool overwrite)
{
    QDir srcDir(src);
    if (!srcDir.exists())
        return false;
    QDir().mkpath(dst);

    for (const QFileInfo &fi :
         srcDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QString target = QDir(dst).absoluteFilePath(fi.fileName());
        if (fi.isDir()) {
            if (!CopyDirectoryFiles(fi.absoluteFilePath(), target, overwrite))
                return false;
        } else {
            if (QFile::exists(target)) {
                if (!overwrite)
                    continue;
                QFile::remove(target);
            }
            if (!QFile::copy(fi.absoluteFilePath(), target))
                return false;
        }
    }
    return true;
}

} // namespace KSystem
