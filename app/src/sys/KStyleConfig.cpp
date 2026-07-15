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

QString KStyleConfig::EncodeScopeName(const QString &scope)
{
    // ConvertSrc2Enc: каждый байт ASCII → 2 hex-символа (нижний регистр).
    return QString::fromLatin1(scope.toLatin1().toHex());
}

int KStyleConfig::GetScopeDefaultCut(const QString &series, const QString &brand,
                                     const QString &scope, int way) const
{
    const QString file = GetStylePath(series, brand) + "/scope/video.ini";
    QSettings ini(file, QSettings::IniFormat);
    const QString key = (way == 0) ? "defaultRoundCut" : "defaultOctangleCut";
    // Реф.: фолбэк — значение из [Default], затем per-scope override.
    const int def = ini.value("Default/" + key, 0).toInt();
    return ini.value(EncodeScopeName(scope) + "/" + key, def).toInt();
}

KStyleConfig::ScopeInfo KStyleConfig::GetScopeInfo(const QString &series,
                                                   const QString &brand,
                                                   const QString &scope) const
{
    ScopeInfo si;
    const QString file = GetStylePath(series, brand) + "/scope/video.ini";
    QSettings ini(file, QSettings::IniFormat);
    const QString enc = EncodeScopeName(scope);
    // Есть ли секция скопа? Иначе — [Default].
    const QString grp = ini.childGroups().contains(enc) ? enc : QString("Default");
    if (!ini.childGroups().contains(grp)) return si;   // нет ни секции, ни Default
    auto val = [&](const char *k){ return ini.value(grp + "/" + QString::fromLatin1(k)); };
    si.roundCut     = val("defaultRoundCut").toInt();
    si.octangleCut  = val("defaultOctangleCut").toInt();
    si.shapeType    = val("shapeType").toString();
    si.sensorType   = val("sensorType").toString();
    si.firmwareType = val("firmwareType").toString();
    si.endoType     = val("endoType").toString();
    si.videoSize    = val("videoSize").toRect();
    si.valid        = true;
    return si;
}
