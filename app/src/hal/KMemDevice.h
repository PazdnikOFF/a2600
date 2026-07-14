#pragma once

// Низкоуровневый доступ к физической памяти FPGA/PL (реф. kmemdevice.cpp, X-2600).
// Оригинальная цепочка записи регистра: KPlControl::WriteValueToPL →
// KMemDevice::WriteDevRegister → WriteRegister (mmap страницы /dev/mem, запись
// 32-бит, ЧТЕНИЕ обратно для верификации, munmap). Библиотека libhal для тракта
// PL/ISP НЕ используется — доступ строго через /dev/mem (подтверждено реверсом).
//
// Коды возврата как в оригинале: 1 — успех (read-back совпал), 2 — ошибка.
class KMemDevice
{
public:
    enum Result { Ok = 1, Fail = 2 };

    KMemDevice() = default;
    ~KMemDevice();

    int  OPenDevice();          // реф. OPenDevice → OpenPhyMem("/dev/mem")
    int  CloseDevice();         // реф. CloseDevice → ClosePhyMem
    bool IsOpen() const { return fd_ > 0; }

    // Записать 32-бит регистр по физ. адресу; при traceOn запись логируется и
    // считается успешной без /dev/mem (десктоп). Возвращает Ok/Fail.
    int  WriteDevRegister(unsigned int physAddr, unsigned int value);
    // Прочитать 32-бит регистр. Возвращает Ok/Fail.
    int  ReadDevRegister(unsigned int physAddr, unsigned int &value);

private:
    // Отобразить страницу, содержащую physAddr; вернуть указатель на слово.
    volatile unsigned int *mapWord(unsigned int physAddr, void *&page, unsigned long &pageLen);

    int fd_ = 0;   // дескриптор /dev/mem (0 = закрыт, как в оригинале)
};
