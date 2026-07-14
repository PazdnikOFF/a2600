#pragma once

#include <QString>

// Пользовательские видеопараметры (реф. класс KUserSet + структура _KUserConf).
// Читаются из presetdata/userpreset/osd.ini через QSettings (как в оригинале
// KUserSet::ReadVideoParamConfig). Поля соответствуют секциям osd.ini.
struct _KUserConf {
    int irisMode     = 0;   // [Iris]Mode           (0=PEAK,1=AVE,2=AUTO)
    int zoomLevel    = 0;   // [Level]Zoom          (индекс Zoom Level1..3)
    int imgDenoise   = 0;   // [Level]ImgDenoise
    int brightnessEQ = 0;   // [Level]BrightnessEQ
    int colorMode    = 0;   // [Color]Mode
    int colorR       = 0;   // [Color]ColorR
    int colorB       = 0;   // [Color]ColorB
    int colorC       = 0;   // [Color]ColorC
    int operationMode= 0;   // [Operation]Mode
    int contrastLevel= 1;   // [Contrast]Level      (0=L,1=M,2=H)
    int imgEnhType   = 0;   // [ImgEnhType]Type
    int enhGear1     = 1;   // [ImEnhGear]EnhType1
    int enhGear2     = 1;   // [ImEnhGear]EnhType2
    int enhGear3     = 1;   // [ImEnhGear]EnhType3
};

class KUserSet
{
public:
    static KUserSet &Instance();   // реф. GetKUserSet

    void InitVideoParamConfig();                                   // читает osd.ini
    void ReadVideoParamConfig(_KUserConf *conf, const QString &path);
    const _KUserConf &GetVideoParamConfig() const { return conf_; }

private:
    KUserSet() = default;
    _KUserConf conf_;
};
