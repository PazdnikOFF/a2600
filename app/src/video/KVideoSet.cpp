#include "video/KVideoSet.h"
#include "video/KVideoParam.h"
#include "ctrl/KPlControl.h"

KVideoSet &KVideoSet::Instance()
{
    static KVideoSet inst;
    return inst;
}

void KVideoSet::SetColEnhLevel(int level)
{
    // реф.: обновить держатель + применить в PL (эквивалент SendColorEnhanceValue).
    KVideoParam::Instance().SetColorEnhLevel(level);
    if (pl_)
        pl_->SetColorEnhParam(level != 0, level);
}
int KVideoSet::GetColEnhLevel() const { return KVideoParam::Instance().ColorEnhLevel(); }

void KVideoSet::SetImgEnhLevel(int level)
{
    KVideoParam::Instance().SetImageEnhLevel(level);
    if (pl_)
        pl_->SetImageEnhValue(level);
}
int KVideoSet::GetImgEnhLevel() const { return KVideoParam::Instance().ImageEnhLevel(); }

void KVideoSet::SetDenoiseLevel(int level)
{
    KVideoParam::Instance().SetDenoise(level);
    if (pl_)
        pl_->SetDenoiseLevel(level);
}
int KVideoSet::GetDenoiseLevel() const { return KVideoParam::Instance().Denoise(); }

void KVideoSet::SetBrightEQLevel(int level)
{
    KVideoParam::Instance().SetBrightEQ(level);
    if (pl_)
        pl_->SetBrightEQEnable(level > 0);
}
int KVideoSet::GetBrightEQLevel() const { return KVideoParam::Instance().BrightEQ(); }

void KVideoSet::SetContrastLevel(int level)
{
    KVideoParam::Instance().SetContrastLevel(level);
}
int KVideoSet::GetContrastLevel() const { return KVideoParam::Instance().ContrastLevel(); }

void KVideoSet::SetColorRValue(int mode, int value)
{
    // реф. RBC-тон: канал R → PL 0xa1870004 (через SetColorR).
    (void)mode;
    KVideoParam::Instance().SetRGain(value);
    if (pl_) pl_->SetColorR(value);
}
void KVideoSet::SetColorBValue(int mode, int value)
{
    (void)mode;
    KVideoParam::Instance().SetBGain(value);
    if (pl_) pl_->SetColorB(value);
}
void KVideoSet::SetColorCValue(int mode, int value)
{
    (void)mode;
    KVideoParam::Instance().SetSGain(value);
    if (pl_) pl_->SetColorC(value);
}

void KVideoSet::SetZoomLevel(int level)
{
    KVideoParam::Instance().SetZoomLevel(level);
    if (pl_)
        pl_->SetZoomValue(static_cast<unsigned>(level));
}
int KVideoSet::GetZoomLevel() const { return KVideoParam::Instance().ZoomLevel(); }

void KVideoSet::SetOperationMode(int mode)
{
    KVideoParam::Instance().SetOperationMode(mode);
    if (pl_)
        pl_->SetVistSwitch(mode == 2 || mode == 3);   // SFI/VIST → спектр. тракт
}
int KVideoSet::GetOperationMode() const { return KVideoParam::Instance().IrisMode(); }

void KVideoSet::ResetVideoParam()
{
    // реф. ResetVideoParam — установить дефолтные уровни.
    KVideoParam &vp = KVideoParam::Instance();
    vp.SetColorEnhLevel(1);
    vp.SetContrastLevel(1);
    vp.SetDenoise(0);
    vp.SetBrightEQ(0);
    vp.SetZoomLevel(0);
}
