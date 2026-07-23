#include "sys/KProjectSet.h"
#include "sys/KStyleConfig.h"

#include <QSettings>

KProjectSet &KProjectSet::GetInstance()
{
    static KProjectSet inst;
    return inst;
}

bool KProjectSet::LoadProject(const QString &projectIniPath)
{
    projectIni_ = projectIniPath;
    QSettings ini(projectIniPath, QSettings::IniFormat);
    projectName_    = ini.value("Option/ProjectName").toString();
    projectID_      = ini.value("Option/ProjectID", 0).toInt();
    theme_          = ini.value("Option/Theme").toString();
    releaseVersion_ = ini.value("Option/ReleaseVersion").toString();
    langMode_       = ini.value("ModuleId/LangMode", 0).toInt();
    // [Product]Series=X-2600,X-2500,… (QSettings авто-split по запятой).
    seriesList_     = ini.value("Product/Series").toStringList();
    return !projectName_.isEmpty();
}

QStringList KProjectSet::GetProductModelList(const QString &series) const
{
    // [Product]<series>=<модель1>,<модель2>… из project.ini.
    QSettings ini(projectIni_, QSettings::IniFormat);
    return ini.value("Product/" + series).toStringList();
}

QStringList KProjectSet::GetReleaseVersionList() const
{
    // Реф. открывает project.ini заново (getProjectPath()), а не берёт кэш LoadProject.
    QSettings ini(projectIni_, QSettings::IniFormat);
    return ini.value("Option/ReleaseVersion", "V2.0").toStringList();
}

bool KProjectSet::LoadProductConfig(const QString &productIniPath)
{
    productIni_ = productIniPath;
    QSettings ini(productIniPath, QSettings::IniFormat);
    // [Function] флаги.
    // ⚠️ ДЕФОЛТЫ ИСПРАВЛЕНЫ 2026-07-23: в реф. (IsZoomEnable @0x656970, IsChbEnable
    // @0x656b10, IsVideoRecordEnable @0x656cb0) все три читаются с QVariant(TRUE) —
    // `mov w1,#1` перед QVariant(bool). У нас стояло false, из-за чего запись видео
    // считалась выключенной и KRecordItem не попадал в корневое OSD-меню.
    zoomEnable_   = ini.value("Function/ZOOM", true).toBool();
    chbEnable_    = ini.value("Function/CHB", true).toBool();
    recordEnable_ = ini.value("Function/RECORD", true).toBool();
    // [Limit] дефолт-уровни/пределы.
    imgEnhLevel_ = ini.value("Limit/ImgEnhLevel", 0).toInt();
    colEnhLevel_ = ini.value("Limit/ColEnhLevel", 0).toInt();
    colRBCMax_   = ini.value("Limit/ColRBCMax", 0).toInt();
    colRBCMin_   = ini.value("Limit/ColRBCMin", 0).toInt();
    zoomMin_     = ini.value("Limit/ZoomMin", 1.0).toDouble();
    zoomMax_     = ini.value("Limit/ZoomMax", 1.0).toDouble();
    zoomStep_    = ini.value("Limit/ZoomStep", 0.1).toDouble();
    // [Firmware] PAPP*=true — присутствующие обрабатывающие приложения.
    papp_.clear();
    ini.beginGroup("Firmware");
    for (const QString &k : ini.childKeys())
        if (ini.value(k).toBool())
            papp_ << k;   // напр. PAPP00, PAPP01…
    ini.endGroup();
    return true;
}

bool KProjectSet::IsShowPAPP(int idx) const
{
    return papp_.contains(QString("PAPP%1").arg(idx, 2, 10, QChar('0')));
}

bool KProjectSet::IsHideQRCode() const
{
    // Реф. @0x655600: `KEncStyle::getCurrentStyle().compare("PyCkeun") == 0`.
    return KStyleConfig::GetInstance().GetCurrentStyle() == QStringLiteral("PyCkeun");
}
