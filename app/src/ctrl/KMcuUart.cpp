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

} // namespace mcu
