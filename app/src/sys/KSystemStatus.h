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
    // Тип изменившегося статуса (первый аргумент SystemStatusChange).
    // ⚠️ КОДЫ ВЫВЕРЕНЫ ДИЗАСМОМ 2026-07-23 (раньше были выдуманы последовательно и НЕ
    // совпадали с прибором: запись шла как 2 вместо 7). Источник — константа `mov w1,#N`
    // перед хвостовым переходом на KSystemStatus::SystemStatusChange @0x8284a8 в каждом
    // сеттере (KSystemStatus::Set* @0x621fb0..0x622430).
    enum StatusType {
        ST_ViewType            = 0,    // SetViewType
        ST_Video               = 1,    // SetVideoStatus
        ST_FullScreen          = 1,    // SetFullScreenUIStatus — ⚠️ ТОТ ЖЕ код 1, что и Video
        ST_EndoDisconnectImage = 2,    // SetEndoDisconnectImageStatus
        ST_Network             = 3,    // SetNetworkStatus
        ST_Freeze              = 4,    // SetFreezeStatus
        ST_Agc                 = 5,    // SetAgcStatus
        ST_CHb                 = 6,    // SetCHbStatus
        ST_Record              = 7,    // SetRecordStatus  (KRecordItem фильтрует ИМЕННО 7)
        ST_Lamp                = 8,    // SetLampStatus
        ST_VlsMode             = 9,    // SetVlsMode
        ST_LowLight            = 10,   // SetLowLightMode
        ST_DimmingType         = 11,   // SetDimmingType
        ST_LightLevel          = 12,   // SetLightLevel
        ST_Brightness          = 13,   // SetImageBrightness
        ST_IsVideoCal          = 14,   // SetIsVideoCal
        ST_MainScreenState     = 15,   // SetMainScreenState
        ST_StopWatch           = 16,   // SetStopWatchStatus
        ST_AirPump             = 17,   // SetAirPumpStatus
    };
    // ⚠️ У этих сеттеров в реф. ВООБЩЕ НЕТ оповещения (тело — только запись поля, 8 байт
    // кода): SetIrisValue @0x622260, SetVideoPlayStatus @0x622420, SetPanelType @0x621fb0,
    // SetWindowID @0x621fb8, SetAutoTestStatus @0x6223a0, SetRatioValue, SetIsEUDEndoType.
    // Поэтому у нас они тоже молчат (раньше слали выдуманные типы).

    static KSystemStatus &GetInstance();   // реф. GetSystemStatus()

    // --- Сеттеры (store + emit SystemStatusChange) ---
    void SetFreezeStatus(int v)          { setField(freeze_, v, ST_Freeze); }
    void SetRecordStatus(int v)          { setField(record_, v, ST_Record); }
    void SetVideoStatus(int v)           { setField(video_, v, ST_Video); }
    void SetVideoPlayStatus(bool v) { videoPlay_ = v ? 1 : 0; }   // реф.: БЕЗ оповещения
    void SetStopWatchStatus(int v)       { setField(stopWatch_, v, ST_StopWatch); }
    void SetLightLevel(int v)            { setField(lightLevel_, v, ST_LightLevel); }
    void SetLampStatus(int v)            { setField(lamp_, v, ST_Lamp); }
    void InitLightStatus()               { light_ = 1; }   // реф.: ST_Light в бинарнике не существует
    void SetIrisValue(unsigned char v) { iris_ = v; }   // реф.: БЕЗ оповещения
    void SetImageBrightness(int v)       { setField(brightness_, v, ST_Brightness); }
    void SetCHbStatus(int v)             { setField(chb_, v, ST_CHb); }
    void SetAgcStatus(int v, bool)       { setField(agc_, v, ST_Agc); }
    void SetDimmingType(int v)           { setField(dimmingType_, v, ST_DimmingType); }
    void SetLowLightMode(int v)          { setField(lowLight_, v, ST_LowLight); }
    void SetVlsMode(int v)               { setField(vlsMode_, v, ST_VlsMode); }   // [0x3c]
    void SetViewType(int v)              { setField(viewType_, v, ST_ViewType); }
    void SetPanelType(int v) { panelType_ = v; }   // реф.: БЕЗ оповещения
    void SetNetworkStatus(int v)         { setField(network_, v, ST_Network); }
    void SetWindowID(int v) { windowID_ = v; }   // реф.: БЕЗ оповещения
    void SetAirPumpStatus(int v)         { setField(airPump_, v, ST_AirPump); }
    void SetAutoTestStatus(int v) { autoTest_ = v; }   // реф.: БЕЗ оповещения
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
    int LowLight() const       { return lowLight_; }   // SS[+0x40]
    int DimmingType() const    { return dimmingType_; }
    int VlsMode() const        { return vlsMode_; }   // спектральный режим
    int ViewType() const       { return viewType_; }
    int PanelType() const      { return panelType_; }
    int NetworkStatus() const  { return network_; }
    bool IsEUDEndoType() const { return isEUDEndo_; }
    bool IsVideoCal() const    { return isVideoCal_; }
    bool IsColorEnable() const { return colorEnable_; }
    bool IsDemoireEnable() const { return demoireEnable_; }
    bool IsToneEnable() const  { return toneEnable_; }

    // Реф.: ChangeUserSet/ChangeSystemSet — ОДНОИНСТРУКЦИОННЫЕ tail-jump'ы
    // в соответствующие сигналы (@0x6222c8 / @0x6222d0). В референсе сигналов
    // ТРИ, а у нас раньше был только SystemStatusChange — добавлены остальные
    // два (нужны KLcdProxy).
    void ChangeUserSet(int type, int value)   { emit UserSetChange(type, value); }
    void ChangeSystemSet(int type, int value) { emit SystemSetChange(type, value); }

signals:
    // реф. SystemStatusChange(int статусТип, int значение).
    void SystemStatusChange(int type, int value);
    void UserSetChange(int type, int value);     // реф. @0x828570
    void SystemSetChange(int type, int value);   // реф. @0x8285a8
    // Реф. @0x8285f8 — «изменилась авторизация прибора» (без аргументов). Испускает
    // KTimeMng::EachDayMC в полночь, когда истёк последний день лицензии; подписчик —
    // KViewSoftEndo::AuthMachineChangeAct.
    void AuthMachineChange();

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
