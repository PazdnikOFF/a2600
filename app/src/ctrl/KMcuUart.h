#pragma once

#include <cstddef>
#include <cstdint>

// Протокол UART-связи с платой МК/HMI (реверс update/cmd/hmiupdate — aarch64 ELF
// с debug_info, исходник hmiupdate.c). Это ground truth последовательного обмена
// с микроконтроллерной платой, полученный ИЗ ПРОШИВКИ (не из прибора):
//
//   • устройство:  /dev/ttyPS1  (реф. InitUartDev @0x401754);
//   • параметры:   115200 8N1   (SetCommSpeed(0x1002=B115200) + SetCommAttr(8,0,1));
//   • контроль:    crc_8 @0x4017ac — рефлективный CRC-8, полином 0xB8, init 0;
//   • кадр:        начинается с маркера 0xAA (реф. CrcCheck @0x401850 cmp #0xaa).
//
// Тот же UART/CRC использует рантайм-приём МК в X2000 (KComDataReceiveThread,
// _data_rec_buf) — здесь вынесены общие ПРОВЕРЯЕМЫЕ примитивы; сам ввод-вывод
// (open/tcsetattr/read/write) — device, за швом.
namespace mcu {

// Параметры порта МК (реф. hmiupdate.c).
constexpr const char *kUartDevice = "/dev/ttyPS1";
constexpr int kBaud     = 115200;   // termios B115200 = 0x1002
constexpr int kDataBits = 8;        // CS8
constexpr int kParity   = 0;        // без чётности
constexpr int kStopBits = 1;        // 1 стоп-бит
constexpr uint8_t kFrameHeader = 0xAA;   // маркер начала кадра

// Реф. crc_8 @0x4017ac — обновление аккумулятора одним байтом:
//   mix = (acc ^ byte) & 1;  acc >>= 1;  if (mix) acc ^= 0xB8;  (8 итераций).
uint8_t Crc8Update(uint8_t acc, uint8_t byte);

// Свёртка по буферу (init 0, как в реф. CrcCheck).
uint8_t Crc8(const uint8_t *data, std::size_t len, uint8_t init = 0);

} // namespace mcu
