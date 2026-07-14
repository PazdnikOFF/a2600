#include "hal/Hal.h"

#include <QtGlobal>
#include <QDebug>

namespace hal {

bool init()
{
#ifdef HAVE_HAL
    const int rc = Hal_Init_Hal();
    if (rc != 0) {
        qWarning() << "Hal_Init_Hal failed, rc =" << rc;
        return false;
    }
    qInfo() << "HAL initialized";
    return true;
#else
    qInfo() << "HAL stub (собрано без libhal) — периферия недоступна";
    return true;
#endif
}

} // namespace hal
