#include "ctrl/KMcuUart.h"

namespace mcu {

uint8_t Crc8Update(uint8_t acc, uint8_t byte)
{
    // Реф. crc_8 @0x4017ac: 8 итераций, полином 0xB8 (рефлективный), сдвиг вправо.
    for (int i = 0; i < 8; ++i) {
        const uint8_t mix = (acc ^ byte) & 1u;
        acc >>= 1;
        if (mix)
            acc ^= 0xB8u;
        byte >>= 1;
    }
    return acc;
}

uint8_t Crc8(const uint8_t *data, std::size_t len, uint8_t init)
{
    uint8_t acc = init;
    for (std::size_t i = 0; i < len; ++i)
        acc = Crc8Update(acc, data[i]);
    return acc;
}

uint8_t Crc8Check(const uint8_t *data, std::size_t len)
{
    // Реф. CrcCheck lcdupdate @0x21e0: len==0 → 0xFF; иначе ~Crc8 & 0xFF.
    if (len == 0)
        return 0xFF;
    return static_cast<uint8_t>(~Crc8(data, len, 0));
}

uint8_t XorChecksum(const uint8_t *data, std::size_t len, uint8_t init)
{
    uint8_t x = init;
    for (std::size_t i = 0; i < len; ++i)
        x ^= data[i];
    return x;
}

} // namespace mcu
