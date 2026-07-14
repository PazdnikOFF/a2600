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

    // Силы усиления по гирам: [ImgEnhStrA/B/Edge]EnhLevel1..3.
    for (int i = 0; i < 3; ++i) {
        const QString k = QString("EnhLevel%1").arg(i + 1);
        conf->imgEnhStrA[i]    = ini.value("ImgEnhStrA/" + k, conf->imgEnhStrA[i]).toInt();
        conf->imgEnhStrB[i]    = ini.value("ImgEnhStrB/" + k, conf->imgEnhStrB[i]).toInt();
        conf->imgEnhStrEdge[i] = ini.value("ImgEnhEdge/" + k, conf->imgEnhStrEdge[i]).toInt();
        // [Zoom]Level1..3 — @Variant(float); QSettings десериализует автоматически.
        conf->zoomScale[i] = ini.value(QString("Zoom/Level%1").arg(i + 1),
                                       conf->zoomScale[i]).toFloat();
    }

    // Коды действий кнопок/педали.
    conf->btnALong  = ini.value("ButtomA/LongPress",  conf->btnALong).toInt();
    conf->btnAShort = ini.value("ButtomA/ShortPress", conf->btnAShort).toInt();
    conf->btnBLong  = ini.value("ButtomB/LongPress",  conf->btnBLong).toInt();
    conf->btnBShort = ini.value("ButtomB/ShortPress", conf->btnBShort).toInt();
    conf->btnMLong  = ini.value("ButtomM/LongPress",  conf->btnMLong).toInt();
    conf->btnMShort = ini.value("ButtomM/ShortPress", conf->btnMShort).toInt();
    conf->footSwitch1 = ini.value("FootSwitch/Switch1", conf->footSwitch1).toInt();
    conf->footSwitch2 = ini.value("FootSwitch/Switch2", conf->footSwitch2).toInt();

    conf->dehazeSwitch = ini.value("Dehaze/Switch", conf->dehazeSwitch).toInt();
    conf->hdrSwitch    = ini.value("HDR/Switch", conf->hdrSwitch).toInt();
}
