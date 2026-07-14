#pragma once

#include <QObject>

// Центральный синглтон рантайм-состояния прибора (реф. KSystemStatus,
// GetSystemStatus(), X-2600). Хранит текущие статусы (freeze/record/свет/диафрагма/
// спектральный режим VLS/вид/панель/сеть…) и оповещает подписчиков сигналом
// SystemStatusChange(тип, значение). Читается всеми подсистемами.
//
// Ключевой факт реверса: VlsMode на смещении [this+0x3c] (реф. SetVlsMode,
// GetVistValue: 2=SFI, 3=VIST) — спектральный режим (совпадает с OperationMode
// WL/EWL/SFI/VIST). Совместимо с KColdLightConfig и SetVistSwitch/Matrix.
class KSystemStatus : public QObject
{
    Q_OBJECT
public:
    // Тип изменившегося статуса (аргумент SystemStatusChange). Значения —
    // стабильные ID для подписчиков (SystemStatusChangeActImpl).
    enum StatusType {
        ST_None = 0, ST_Freeze, ST_Record, ST_Video, ST_VideoPlay, ST_StopWatch,
        ST_LightLevel, ST_Lamp, ST_Light, ST_Iris, ST_Brightness, ST_CHb, ST_Agc,
        ST_DimmingType, ST_LowLight, ST_VlsMode, ST_ViewType, ST_PanelType,
        ST_Network, ST_WindowID, ST_AirPump, ST_AutoTest, ST_FullScreen,
        ST_EndoDisconnectImage, ST_ForceLogout
    };

    static KSystemStatus &GetInstance();   // реф. GetSystemStatus()

    // --- Сеттеры (store + emit SystemStatusChange) ---
    void SetFreezeStatus(int v)          { setField(freeze_, v, ST_Freeze); }
    void SetRecordStatus(int v)          { setField(record_, v, ST_Record); }
    void SetVideoStatus(int v)           { setField(video_, v, ST_Video); }
    void SetVideoPlayStatus(bool v)      { setField(videoPlay_, v ? 1 : 0, ST_VideoPlay); }
    void SetStopWatchStatus(int v)       { setField(stopWatch_, v, ST_StopWatch); }
    void SetLightLevel(int v)            { setField(lightLevel_, v, ST_LightLevel); }
    void SetLampStatus(int v)            { setField(lamp_, v, ST_Lamp); }
    void InitLightStatus()               { setField(light_, 1, ST_Light); }
    void SetIrisValue(unsigned char v)   { setField(iris_, v, ST_Iris); }
    void SetImageBrightness(int v)       { setField(brightness_, v, ST_Brightness); }
    void SetCHbStatus(int v)             { setField(chb_, v, ST_CHb); }
    void SetAgcStatus(int v, bool)       { setField(agc_, v, ST_Agc); }
    void SetDimmingType(int v)           { setField(dimmingType_, v, ST_DimmingType); }
    void SetLowLightMode(int v)          { setField(lowLight_, v, ST_LowLight); }
    void SetVlsMode(int v)               { setField(vlsMode_, v, ST_VlsMode); }   // [0x3c]
    void SetViewType(int v)              { setField(viewType_, v, ST_ViewType); }
    void SetPanelType(int v)             { setField(panelType_, v, ST_PanelType); }
    void SetNetworkStatus(int v)         { setField(network_, v, ST_Network); }
    void SetWindowID(int v)              { setField(windowID_, v, ST_WindowID); }
    void SetAirPumpStatus(int v)         { setField(airPump_, v, ST_AirPump); }
    void SetAutoTestStatus(int v)        { setField(autoTest_, v, ST_AutoTest); }
    void SetFullScreenUIStatus(int v)    { setField(fullScreen_, v, ST_FullScreen); }
    void SetEndoDisconnectImageStatus(int v) { setField(endoDiscImage_, v, ST_EndoDisconnectImage); }
    void SetIsEUDEndoType(bool v)        { isEUDEndo_ = v; }
    void SetIsVideoCal(bool v)           { isVideoCal_ = v; }
    void SetRatioValue(double x, double y) { ratioX_ = x; ratioY_ = y; }

    // Флаги-переключатели (реф. IsColorEnable/IsDemoireEnable/IsToneEnable).
    void SetColorEnable(bool v)          { colorEnable_ = v; }
    void SetDemoireEnable(bool v)        { demoireEnable_ = v; }
    void SetToneEnable(bool v)           { toneEnable_ = v; }

    // --- Геттеры ---
    int FreezeStatus() const   { return freeze_; }
    int RecordStatus() const   { return record_; }
    int VideoStatus() const    { return video_; }
    int LightLevel() const     { return lightLevel_; }
    int IrisValue() const      { return iris_; }
    int ImageBrightness() const { return brightness_; }
    int CHbStatus() const      { return chb_; }
    int VlsMode() const        { return vlsMode_; }   // спектральный режим
    int ViewType() const       { return viewType_; }
    int PanelType() const      { return panelType_; }
    int NetworkStatus() const  { return network_; }
    bool IsEUDEndoType() const { return isEUDEndo_; }
    bool IsVideoCal() const    { return isVideoCal_; }
    bool IsColorEnable() const { return colorEnable_; }
    bool IsDemoireEnable() const { return demoireEnable_; }
    bool IsToneEnable() const  { return toneEnable_; }

signals:
    // реф. SystemStatusChange(int статусТип, int значение).
    void SystemStatusChange(int type, int value);

private:
    explicit KSystemStatus(QObject *parent = nullptr) : QObject(parent) {}
    void setField(int &field, int value, StatusType type) {
        if (field == value) return;   // без изменения — не оповещаем
        field = value;
        emit SystemStatusChange(type, value);
    }

    int freeze_ = 0, record_ = 0, video_ = 0, videoPlay_ = 0, stopWatch_ = 0;
    int lightLevel_ = 0, lamp_ = 0, light_ = 0, iris_ = 0, brightness_ = 0;
    int chb_ = 0, agc_ = 0, dimmingType_ = 0, lowLight_ = 0, vlsMode_ = 0;
    int viewType_ = 0, panelType_ = 0, network_ = 0, windowID_ = 0;
    int airPump_ = 0, autoTest_ = 0, fullScreen_ = 0, endoDiscImage_ = 0;
    bool isEUDEndo_ = false, isVideoCal_ = false;
    bool colorEnable_ = true, demoireEnable_ = false, toneEnable_ = false;
    double ratioX_ = 1.0, ratioY_ = 1.0;
};
