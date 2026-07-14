#pragma once

#include <QString>
#include <QStringList>

// Продуктовая конфигурация (реф. KProjectSet, X-2600). Читает ДВА конфига:
//   • глобальный display/project.ini — [Option] ProjectName/ProjectID/Theme/
//     ReleaseVersion; [Product] Series + списки моделей; [ModuleId] LangMode;
//   • per-модель display/<SERIES>/<MODEL>/product.ini (реф. GetProductConfig →
//     KSystem::ProductDisplayConfigFile) — [Function] флаги ZOOM/CHB/RECORD;
//     [Limit] уровни ImgEnh/ColEnh/ColRBCMax/Min + Zoom; [Firmware] PAPP00..80.
// Фича-флаги гейтят соответствующие функции (CHB→SetChbStatus, ZOOM→SetZoomValue…).
class KProjectSet
{
public:
    static KProjectSet &GetInstance();

    bool LoadProject(const QString &projectIniPath);        // глобальный project.ini
    bool LoadProductConfig(const QString &productIniPath);  // per-модель product.ini

    // --- project.ini ---
    QString GetProjectName() const { return projectName_; }
    int     GetProjectID() const   { return projectID_; }
    QString GetThemeName() const   { return theme_; }
    QString GetReleaseVersion() const { return releaseVersion_; }
    int     LanguageMode() const   { return langMode_; }
    QStringList GetProductSeriesList() const { return seriesList_; }
    QStringList GetProductModelList(const QString &series) const;

    // --- product.ini: [Function] флаги ---
    bool IsZoomEnable() const        { return zoomEnable_; }
    bool IsChbEnable() const         { return chbEnable_; }
    bool IsVideoRecordEnable() const { return recordEnable_; }
    // [Firmware] PAPP<idx> присутствует (idx: 0,1,2,3,4,6,80).
    bool IsShowPAPP(int idx) const;

    // --- product.ini: [Limit] дефолт-уровни/пределы ---
    int    GetImgEnhLevel() const { return imgEnhLevel_; }
    int    GetColEnhLevel() const { return colEnhLevel_; }
    int    GetColRBCMax() const   { return colRBCMax_; }
    int    GetColRBCMin() const   { return colRBCMin_; }
    double GetZoomMin() const { return zoomMin_; }
    double GetZoomMax() const { return zoomMax_; }
    double GetZoomStep() const { return zoomStep_; }
    bool   IsZoomEnableByLimit() const { return zoomMax_ > zoomMin_; }

private:
    KProjectSet() = default;
    QString projectIni_, productIni_;

    // project.ini
    QString projectName_, theme_, releaseVersion_;
    int projectID_ = 0, langMode_ = 0;
    QStringList seriesList_;

    // product.ini
    bool zoomEnable_ = false, chbEnable_ = false, recordEnable_ = false;
    QStringList papp_;
    int imgEnhLevel_ = 0, colEnhLevel_ = 0, colRBCMax_ = 0, colRBCMin_ = 0;
    double zoomMin_ = 1.0, zoomMax_ = 1.0, zoomStep_ = 0.1;
};
