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

    // Силы усиления по гирам (реф. [ImgEnhStrA/B/Edge]EnhLevel1..3): A=яркость,
    // B=цвет, Edge=край; 3 уровня-гира. Питают SetImageEnhValue.
    int imgEnhStrA[3] = {0,0,0};
    int imgEnhStrB[3] = {0,0,0};
    int imgEnhStrEdge[3] = {0,0,0};

    // Масштабы Zoom (реф. [Zoom]Level1..3 = @Variant float, напр. 1.0/1.2/1.4).
    float zoomScale[3] = {1.0f, 1.0f, 1.0f};

    // Коды действий кнопок/педали (реф. [ButtomA/B/M]LongPress/ShortPress,
    // [FootSwitch]Switch1/2). Хранятся как есть; семантику назначает KMainCtrlThread.
    int btnALong = 0, btnAShort = 0;
    int btnBLong = 0, btnBShort = 0;
    int btnMLong = 0, btnMShort = 0;
    int footSwitch1 = 0, footSwitch2 = 0;

    int dehazeSwitch = 0;   // [Dehaze]Switch
    int hdrSwitch    = 0;   // [HDR]Switch
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
