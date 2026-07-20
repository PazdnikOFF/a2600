#pragma once

#include <QString>

// USB-накопитель (реф. KUsbDevice, X-2600). На устройстве определяет точку
// монтирования флешки; здесь — off-device часть: хранение и выдача пути
// (GetUsbPath), которую использует KExamBussinessHandler::GetSaveDataPath.
class KUsbDevice
{
public:
    static KUsbDevice *GetInstance();

    QString GetUsbPath() const { return m_usbPath; }
    // На устройстве путь ставит детектор hotplug; off-device — self-test.
    void    SetUsbPath(const QString &p) { m_usbPath = p; }
    bool    IsUsbDisconnect() const { return m_usbPath.isEmpty(); }

private:
    KUsbDevice() = default;
    QString m_usbPath;
};
