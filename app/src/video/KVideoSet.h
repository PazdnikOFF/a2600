#pragma once

class KPlControl;

// Высокоуровневая оркестрация видеопараметров (реф. KVideoSet, X-2600). Мост между
// UI/меню и трактом: Set*Level → обновить держатель KVideoParam + применить в PL
// через KPlControl (эквивалент KVideoProxy::Send* на устройстве). Get* — из KVideoParam.
// Имена методов 1:1. Значения уровней резолвятся через AlgParaManager (не хардкод).
class KVideoSet
{
public:
    static KVideoSet &Instance();
    void AttachPl(KPlControl *pl) { pl_ = pl; }

    // --- Цветоусиление / изображение ---
    void SetColEnhLevel(int level);        // → KVideoParam + SetColorEnhParam
    int  GetColEnhLevel() const;
    void SetImgEnhLevel(int level);        // → SetImageEnhValue
    int  GetImgEnhLevel() const;
    void SetDenoiseLevel(int level);       // → SetDenoiseLevel
    int  GetDenoiseLevel() const;
    void SetBrightEQLevel(int level);      // → SetBrightEQEnable
    int  GetBrightEQLevel() const;
    void SetContrastLevel(int level);
    int  GetContrastLevel() const;

    // --- RBC-тон (реф. SetColorR/B/CValue → PL 0xa1870004/8/0) ---
    void SetColorRValue(int mode, int value);
    void SetColorBValue(int mode, int value);
    void SetColorCValue(int mode, int value);

    // --- Zoom / режим ---
    void SetZoomLevel(int level);          // → SetZoomValue
    int  GetZoomLevel() const;
    void SetOperationMode(int mode);       // WL/EWL/SFI/VIST
    int  GetOperationMode() const;

    void ResetVideoParam();                // реф. ResetVideoParam — дефолты

private:
    KVideoSet() = default;
    KPlControl *pl_ = nullptr;
};
