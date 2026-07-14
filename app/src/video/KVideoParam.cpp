#include "video/KVideoParam.h"
#include "sys/KUserSet.h"

KVideoParam &KVideoParam::Instance()
{
    static KVideoParam inst;
    return inst;
}

void KVideoParam::InitFromUserConf(const _KUserConf &c)
{
    // Перенос значений из osd.ini (реф. UpdateVideoParam после ReadVideoParamConfig).
    irisMode_      = c.irisMode;
    zoomLevel_     = c.zoomLevel;
    imgEnhLevel_   = c.imgEnhType;
    colorEnhLevel_ = c.enhGear1;
    contrastLevel_ = c.contrastLevel;
    rbcMode_       = c.colorR;       // Col RBC (режим красного)
    brightEQ_      = c.brightnessEQ;
    denoise_       = c.imgDenoise;
    operationMode_ = c.operationMode;
}
