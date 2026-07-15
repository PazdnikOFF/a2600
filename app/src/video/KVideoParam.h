#pragma once

#include <QString>

struct _KUserConf;

// Держатель живых видеопараметров (реф. класс KVideoParam, X-2600).
// Инициализируется из пользовательского конфига (KUserSet/_KUserConf, osd.ini),
// отдаёт значения в OSD и в FPGA (через KVideoProxy/KPlControl).
// Реальные методы: SetIrisMode, SetZoomLevel, SetImageEnhLevel/Mode,
// SetColorEnhLevel, SetContrastLevel, SetRBCMode/SetRGain/SetBGain/SetSGain,
// SetBrightEQ, SetDenoise, SetDehaze, SetDemoire, SetHDR, SetOperationMode.
class KVideoParam
{
public:
    static KVideoParam &Instance();

    void InitFromUserConf(const _KUserConf &c);   // реф. UpdateVideoParam

    // Сеттеры (реф. имена)
    void SetIrisMode(int v)       { irisMode_ = v; }
    void SetZoomLevel(int v)      { zoomLevel_ = v; }
    void SetImageEnhLevel(int v)  { imgEnhLevel_ = v; }
    void SetColorEnhLevel(int v)  { colorEnhLevel_ = v; }
    void SetContrastLevel(int v)  { contrastLevel_ = v; }
    void SetRBCMode(int v)        { rbcMode_ = v; }
    void SetRGain(int v)          { rGain_ = v; }
    void SetBGain(int v)          { bGain_ = v; }
    void SetSGain(int v)          { sGain_ = v; }
    void SetBrightEQ(int v)       { brightEQ_ = v; }
    void SetDenoise(int v)        { denoise_ = v; }
    void SetOperationMode(int v)  { operationMode_ = v; }
    void SetRBCBase(int v)        { rbcBase_ = v; }
    void SetDemoire(int v)        { demoire_ = v; }    // статус подавления муара (0/1)
    void SetDehaze(int v)         { dehaze_ = v; }     // статус удаления дымки (0/1)
    void SetHDR(int v)            { hdr_ = v; }        // статус HDR (0/1)

    // Геттеры
    int IrisMode()      const { return irisMode_; }
    int ZoomLevel()     const { return zoomLevel_; }
    int ImageEnhLevel() const { return imgEnhLevel_; }
    int ColorEnhLevel() const { return colorEnhLevel_; }
    int ContrastLevel() const { return contrastLevel_; }
    int RBCMode()       const { return rbcMode_; }
    int RGain()         const { return rGain_; }
    int BGain()         const { return bGain_; }
    int SGain()         const { return sGain_; }
    int BrightEQ()      const { return brightEQ_; }
    int Denoise()       const { return denoise_; }
    int RBCBase()       const { return rbcBase_; }   // базовый уровень тона RBC
    int DemoireStatus() const { return demoire_; }   // статус подавления муара
    int DehazeStatus()  const { return dehaze_; }    // статус удаления дымки
    int HDRStatus()     const { return hdr_; }       // статус HDR

private:
    KVideoParam() = default;
    int irisMode_ = 0, zoomLevel_ = 0, imgEnhLevel_ = 0, colorEnhLevel_ = 1;
    int contrastLevel_ = 1, rbcMode_ = 0, rGain_ = 0, bGain_ = 0, sGain_ = 0;
    int brightEQ_ = 0, denoise_ = 0, operationMode_ = 0, rbcBase_ = 0;
    int demoire_ = 0, dehaze_ = 0, hdr_ = 0;
};
