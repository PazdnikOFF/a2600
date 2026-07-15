#include "sys/KEncStyle.h"
#include "sys/KStyleConfig.h"

#include <QSettings>
#include <QFileInfo>
#include <QDir>

bool KEncStyle::Load(const QString &matchedScopeIniPath)
{
    if (!QFileInfo::exists(matchedScopeIniPath)) {
        iniPath_.clear();
        return false;
    }
    iniPath_ = matchedScopeIniPath;
    return true;
}

bool KEncStyle::LoadForStyle(const QString &series, const QString &brand)
{
    // Путь: <style>/<series>/<brand>/scope/matchedScope.ini (реф. getScopeInfoPath).
    const QString stylePath = KStyleConfig::GetInstance().GetStylePath(series, brand);
    return Load(QDir(stylePath).absoluteFilePath("scope/matchedScope.ini"));
}

QStringList KEncStyle::listFor(const QString &key) const
{
    if (iniPath_.isEmpty())
        return {};
    QSettings ini(iniPath_, QSettings::IniFormat);
    // Реф.: ключ "<Model>/<key>" если присутствует, иначе фолбэк "Default/<key>".
    const QString modelKey = productModel_ + "/" + key;
    const QString useKey = ini.contains(modelKey) ? modelKey : ("Default/" + key);
    // Нативный Qt-INI разбор списка по запятой (реф. QVariant::toStringList).
    return ini.value(useKey).toStringList();
}

QStringList KEncStyle::getSupportedScopeList() const
{
    return listFor("scope");
}

QStringList KEncStyle::GetSupprotedCameraList() const
{
    return listFor("camera");
}

bool KEncStyle::IsScopeValid(const QString &scope) const
{
    // Реф.-нормализация: "…LT…" вместе с "EC" → "…L/T…" (чистая строка).
    // ("*N"-суффикс и кросс-проверка с глобальным enc-списком getScopeList — device/отложено.)
    QString s = scope;
    if (s.contains("LT") && s.contains("EC"))
        s.replace("LT", "L/T");
    return getSupportedScopeList().contains(s, Qt::CaseSensitive);
}

bool KEncStyle::IsCameraValid(const QString &camera) const
{
    // Реф.: членство в списке камер (без нормализации), case-sensitive.
    return GetSupprotedCameraList().contains(camera, Qt::CaseSensitive);
}
