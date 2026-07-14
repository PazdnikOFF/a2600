#include "hal/KMemDevice.h"

#if defined(__linux__)
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#endif

KMemDevice::~KMemDevice()
{
    CloseDevice();
}

int KMemDevice::OPenDevice()
{
#if defined(__linux__)
    if (fd_ > 0) return Ok;
    fd_ = ::open("/dev/mem", O_RDWR | O_SYNC);   // реф. OpenPhyMem
    return fd_ > 0 ? Ok : Fail;
#else
    return Fail;   // десктоп: физпамяти нет
#endif
}

int KMemDevice::CloseDevice()
{
#if defined(__linux__)
    if (fd_ > 0) { ::close(fd_); fd_ = 0; }   // реф. ClosePhyMem
#endif
    return Ok;
}

volatile unsigned int *KMemDevice::mapWord(unsigned int physAddr, void *&page,
                                           unsigned long &pageLen)
{
#if defined(__linux__)
    // реф. WriteRegister: getpagesize; mmap(0,pg,RW,SHARED,fd,addr & ~(pg-1)).
    const unsigned long pg = static_cast<unsigned long>(::getpagesize());
    const unsigned long base = physAddr & ~(pg - 1);
    void *p = ::mmap(nullptr, pg, PROT_READ | PROT_WRITE, MAP_SHARED, fd_,
                     static_cast<off_t>(base));
    if (p == MAP_FAILED) return nullptr;
    page = p; pageLen = pg;
    return reinterpret_cast<volatile unsigned int *>(
        static_cast<char *>(p) + (physAddr & (pg - 1)));
#else
    (void)physAddr; (void)page; (void)pageLen;
    return nullptr;
#endif
}

int KMemDevice::WriteDevRegister(unsigned int physAddr, unsigned int value)
{
    // реф. WriteDevRegister: если не открыт — OPenDevice; затем WriteRegister
    // с чтением обратно и сравнением (Ok при совпадении, иначе Fail).
    if (fd_ <= 0 && OPenDevice() != Ok)
        return Fail;
#if defined(__linux__)
    void *page = nullptr; unsigned long pageLen = 0;
    volatile unsigned int *reg = mapWord(physAddr, page, pageLen);
    if (!reg) return Fail;
    *reg = value;
    const unsigned int readback = *reg;   // верификация записи
    ::munmap(page, pageLen);
    return readback == value ? Ok : Fail;
#else
    (void)value;
    return Fail;
#endif
}

int KMemDevice::ReadDevRegister(unsigned int physAddr, unsigned int &value)
{
    if (fd_ <= 0 && OPenDevice() != Ok)
        return Fail;
#if defined(__linux__)
    void *page = nullptr; unsigned long pageLen = 0;
    volatile unsigned int *reg = mapWord(physAddr, page, pageLen);
    if (!reg) return Fail;
    value = *reg;
    ::munmap(page, pageLen);
    return Ok;
#else
    (void)physAddr; (void)value;
    return Fail;
#endif
}
