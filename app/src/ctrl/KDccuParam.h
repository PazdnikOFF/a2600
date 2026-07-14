#pragma once

#include <QString>
#include <QVariant>

// Хранилище параметров DCCU (Digital Camera Control Unit) — реф. класс KDccuParam
// (X-2600). Значения персистятся в QSettings-ini:
//   SystemPath()/presetdata/syspreset/dccuparam.ini
// (файла нет в дистрибутиве — создаётся в рантайме первой записью).
//
// Ключи секций/имён — 1:1 из дизассемблера ReadDccuParam/WriteDccuParam:
//   AEC/Control, AEC/AutoAEC, AEC/Min, AEC/Max, AEC/GradpUp/Down, AEC/Gradthresh*,
//   AEC/ManualOV2740, AEC/ManualOV9734, AGC/Control, AGC/Manual, AGC/AutoAGC,
//   AGC/Min, AGC/Max, AGC/Gradp*, AGC/Gradthresh*, AGC/3AMaxAGC,
//   AWB/IsCorrectionOn, AWB/IsAssessOn, AWB/Up, AWB/Down,
//   RB/IsOpen, RB/Hb, RB/Hr, RB/Satiation, CCM/IsOpen, CCM/Matrix,
//   ElectronicFilter/IsOpen, ElectronicFilter/Matrix, Gamma/IsOpen/Bp/Ratio,
//   Zoom/IsOpen/Ratio.
//
// Значения питают авто-режимы экспозиции/усиления/ББ (K3ADimming) и далее уходят
// в камеру/PL через KVideoProxy::SetAEC/AGC/AWBValue.
class KDccuParam
{
public:
    static KDccuParam &GetInstance();

    // Базовые примитивы доступа (реф. ReadDccuParam/WriteDccuParam).
    QVariant ReadDccuParam(const QString &key, const QVariant &def) const;
    void     WriteDccuParam(const QString &key, const QVariant &value);

    // --- AEC ---
    int  GetAECControl() const     { return ReadDccuParam("AEC/Control", 0).toInt(); }
    void SetAECControl(int v)      { WriteDccuParam("AEC/Control", v); }
    int  GetAutoAEC() const        { return ReadDccuParam("AEC/AutoAEC", 1).toInt(); }
    void SetAutoAEC(int v)         { WriteDccuParam("AEC/AutoAEC", v); }
    int  GetAECMin() const         { return ReadDccuParam("AEC/Min", 0).toInt(); }
    void SetAECMin(int v)          { WriteDccuParam("AEC/Min", v); }
    int  GetAECMax() const         { return ReadDccuParam("AEC/Max", 0).toInt(); }
    void SetAECMax(int v)          { WriteDccuParam("AEC/Max", v); }
    double GetManualAECOV2740() const { return ReadDccuParam("AEC/ManualOV2740", 0.0).toDouble(); }
    void SetManualAECOV2740(double v) { WriteDccuParam("AEC/ManualOV2740", v); }
    double GetManualAECOV9734() const { return ReadDccuParam("AEC/ManualOV9734", 0.0).toDouble(); }
    void SetManualAECOV9734(double v) { WriteDccuParam("AEC/ManualOV9734", v); }

    // --- AGC ---
    int  GetAGCControl() const     { return ReadDccuParam("AGC/Control", 0).toInt(); }
    void SetAGCControl(int v)      { WriteDccuParam("AGC/Control", v); }
    int  GetAutoAGC() const        { return ReadDccuParam("AGC/AutoAGC", 1).toInt(); }
    void SetAutoAGC(int v)         { WriteDccuParam("AGC/AutoAGC", v); }
    double GetManualAGC() const    { return ReadDccuParam("AGC/Manual", 0.0).toDouble(); }
    void SetManualAGC(double v)    { WriteDccuParam("AGC/Manual", v); }
    int  GetAGCMin() const         { return ReadDccuParam("AGC/Min", 0).toInt(); }
    void SetAGCMin(int v)          { WriteDccuParam("AGC/Min", v); }
    int  GetAGCMax() const         { return ReadDccuParam("AGC/Max", 0).toInt(); }
    void SetAGCMax(int v)          { WriteDccuParam("AGC/Max", v); }
    int  Get3ADimmingAGCMax() const { return ReadDccuParam("AGC/3AMaxAGC", 0).toInt(); }
    void Set3ADimmingAGCMax(int v)  { WriteDccuParam("AGC/3AMaxAGC", v); }

    // --- AWB ---
    bool GetAwbCorrectionStatus() const { return ReadDccuParam("AWB/IsCorrectionOn", false).toBool(); }
    void SetAwbCorrectionStatus(bool v) { WriteDccuParam("AWB/IsCorrectionOn", v); }
    bool GetAwbAssessStatus() const     { return ReadDccuParam("AWB/IsAssessOn", false).toBool(); }
    void SetAwbAssessStatus(bool v)     { WriteDccuParam("AWB/IsAssessOn", v); }
    int  GetAwbUp() const          { return ReadDccuParam("AWB/Up", 0).toInt(); }
    void SetAwbUp(int v)           { WriteDccuParam("AWB/Up", v); }
    int  GetAwbDown() const        { return ReadDccuParam("AWB/Down", 0).toInt(); }
    void SetAwbDown(int v)         { WriteDccuParam("AWB/Down", v); }

    // --- RB (усиление сосудов) ---
    bool GetRBStatus() const       { return ReadDccuParam("RB/IsOpen", false).toBool(); }
    void SetRBStatus(bool v)       { WriteDccuParam("RB/IsOpen", v); }
    int  GetRBHb() const           { return ReadDccuParam("RB/Hb", 0).toInt(); }
    void SetRBHb(int v)            { WriteDccuParam("RB/Hb", v); }
    int  GetRBHr() const           { return ReadDccuParam("RB/Hr", 0).toInt(); }
    void SetRBHr(int v)            { WriteDccuParam("RB/Hr", v); }
    int  GetRBSatiation() const    { return ReadDccuParam("RB/Satiation", 0).toInt(); }
    void SetRBSatiation(int v)     { WriteDccuParam("RB/Satiation", v); }

    // --- Gamma / Zoom / CCM статусы ---
    bool GetGammaStatus() const    { return ReadDccuParam("Gamma/IsOpen", false).toBool(); }
    void SetGammaStatus(bool v)    { WriteDccuParam("Gamma/IsOpen", v); }
    int  GetGammaBp() const        { return ReadDccuParam("Gamma/Bp", 0).toInt(); }
    void SetGammaBp(int v)         { WriteDccuParam("Gamma/Bp", v); }
    double GetGammaRatio() const   { return ReadDccuParam("Gamma/Ratio", 1.0).toDouble(); }
    void SetGammaRatio(double v)   { WriteDccuParam("Gamma/Ratio", v); }
    bool GetZoomStatus() const     { return ReadDccuParam("Zoom/IsOpen", false).toBool(); }
    void SetZoomStatus(bool v)     { WriteDccuParam("Zoom/IsOpen", v); }
    double GetZoomRatio() const    { return ReadDccuParam("Zoom/Ratio", 1.0).toDouble(); }
    void SetZoomRatio(double v)    { WriteDccuParam("Zoom/Ratio", v); }
    bool GetCCMStatus() const      { return ReadDccuParam("CCM/IsOpen", false).toBool(); }
    void SetCCMStatus(bool v)      { WriteDccuParam("CCM/IsOpen", v); }

    // Путь ini (для тестов/диагностики).
    QString ConfigFile() const;

private:
    KDccuParam() = default;
};
