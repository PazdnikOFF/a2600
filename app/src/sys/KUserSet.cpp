#include "sys/KUserSet.h"
#include "sys/KSystem.h"

#include <QSettings>
#include <QDir>

KUserSet &KUserSet::Instance()
{
    static KUserSet inst;
    return inst;
}

void KUserSet::InitVideoParamConfig()
{
    // Оригинал: KUserSet::InitVideoParamConfig → ReadVideoParamConfig(osd.ini)
    ReadVideoParamConfig(&conf_, QDir(KSystem::UserPresetPath()).absoluteFilePath("osd.ini"));
}

void KUserSet::ReadVideoParamConfig(_KUserConf *conf, const QString &path)
{
    if (!conf) return;
    QSettings ini(path, QSettings::IniFormat);
    // Ключи — как в osd.ini прошивки (QSettings::value().toInt()).
    conf->irisMode      = ini.value("Iris/Mode", conf->irisMode).toInt();
    conf->zoomLevel     = ini.value("Level/Zoom", conf->zoomLevel).toInt();
    conf->imgDenoise    = ini.value("Level/ImgDenoise", conf->imgDenoise).toInt();
    conf->brightnessEQ  = ini.value("Level/BrightnessEQ", conf->brightnessEQ).toInt();
    conf->colorMode     = ini.value("Color/Mode", conf->colorMode).toString().section(',', 0, 0).toInt();
    conf->colorR        = ini.value("Color/ColorR", 0).toString().section(',', 0, 0).toInt();
    conf->colorB        = ini.value("Color/ColorB", 0).toString().section(',', 0, 0).toInt();
    conf->colorC        = ini.value("Color/ColorC", 0).toString().section(',', 0, 0).toInt();
    conf->operationMode = ini.value("Operation/Mode", conf->operationMode).toInt();
    conf->contrastLevel = ini.value("Contrast/Level", conf->contrastLevel).toInt();
    conf->imgEnhType    = ini.value("ImgEnhType/Type", conf->imgEnhType).toInt();
    conf->enhGear1      = ini.value("ImEnhGear/EnhType1", conf->enhGear1).toInt();
    conf->enhGear2      = ini.value("ImEnhGear/EnhType2", conf->enhGear2).toInt();
    conf->enhGear3      = ini.value("ImEnhGear/EnhType3", conf->enhGear3).toInt();
}
