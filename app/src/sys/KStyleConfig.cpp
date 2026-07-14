#include "sys/KStyleConfig.h"
#include "sys/KSystem.h"

#include <QSettings>
#include <QDir>

KStyleConfig &KStyleConfig::GetInstance()
{
    static KStyleConfig inst;
    return inst;
}

QString KStyleConfig::styleRoot() const
{
    if (!styleRoot_.isEmpty())
        return styleRoot_;
    return QDir(KSystem::SystemPath()).absoluteFilePath("style");
}

bool KStyleConfig::Load()
{
    QSettings ini(QDir(styleRoot()).absoluteFilePath("stylelist.ini"), QSettings::IniFormat);
    // [Style]StyleList=SonoScapeCN,SonoScape,PyCkeun,SonoScapeHK (QSettings авто-split).
    styleList_.clear();
    for (const QString &s : ini.value("Style/StyleList").toStringList())
        if (!s.trimmed().isEmpty())
            styleList_ << s.trimmed();
    return !styleList_.isEmpty();
}

QString KStyleConfig::GetStylePath(const QString &series, const QString &brand) const
{
    return QDir(styleRoot()).absoluteFilePath(series + "/" + brand);
}
