#include "hal/KUsbDevice.h"

KUsbDevice *KUsbDevice::GetInstance()
{
    static KUsbDevice inst;
    return &inst;
}
