#include "video/KVideoCal.h"
#include "ui/KDisplayOption.h"

QPair<int, int> KVideoCal::GetCenterOffsetHorizontalRange(int fw)
{
    // 1:1 с X2000: _ZN9KVideoCal30GetCenterOffsetHorizontalRangeE17_EndoFirmwareType
    switch (fw) {
    case FW_OH01A_928X768:    // 1
    case FW_OH01A_768X928:    // 3
    case FW_OV2740_1280X960:  // 5
    case FW_OV2740_1024X1024: // 8
        return {-16, 16};
    case FW_OV2740:           // 0
        return {-4, 4};
    default:
        return {0, 0};
    }
}

QPair<int, int> KVideoCal::GetCenterOffsetVerticalRange(int fw)
{
    // 1:1 с X2000: _ZN9KVideoCal28GetCenterOffsetVerticalRangeE17_EndoFirmwareType
    switch (fw) {
    case FW_OH01A_928X768:    // 1
    case FW_OH01A_768X928:    // 3
        return {-10, 10};
    case FW_OV2740_1280X960:  // 5
    case FW_OV2740_1024X1024: // 8
        return {-16, 16};
    case FW_OV2740:           // 0
        return {-4, 4};
    default:
        return {0, 0};
    }
}

bool KVideoCal::SaveDisplayArea(const QRect &imgPro, const QRect &ui)
{
    KDisplayOption &opt = KDisplayOption::Instance();
    if (opt.LayoutFile().isEmpty())
        return false;
    opt.setVideoRectForImgPro(imgPro);  // [VIDEO]/IMAGE
    opt.setVideoRectForUI(ui);          // [UI]/IMAGE
    return true;
}
