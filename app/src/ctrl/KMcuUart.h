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

// Кадровый CRC (реф. CrcCheck lcdupdate @0x21e0): комплемент свёртки — ~Crc8 & 0xFF;
// для пустого буфера возвращает 0xFF. Именно это значение кладётся в кадр.
uint8_t Crc8Check(const uint8_t *data, std::size_t len);

// Бегущий XOR по байтам (хвостовой чек-байт кадра lcdupdate).
uint8_t XorChecksum(const uint8_t *data, std::size_t len, uint8_t init = 0);

// --- Кадр управляющего канала МК/LCD (реверс lcdupdate SendOneMsgToUart @0x25b0) ---
// Раскладка (подтверждена дизасмом):
//   [0]      0xAA                 маркер;
//   [1..2]   cmd, BIG-ENDIAN      (rev16: старший байт первым);
//   [3]      len                  длина payload;
//   [4..]    payload[len];
//   [4+len]  Crc8Check(payload)   комплементированный CRC-8 poly 0xB8;
//   [5+len]  XOR{len, cmdHi, cmdLo, payload…, crc}.
// Коды команд обновления (реф. main/NotifyMcuToUpdate lcdupdate):
enum FrameCmd : uint16_t {
    kCmdUpdateStart  = 0x0444,   // «войти в режим обновления» (UART)
    kCmdUpdateAck    = 0x8444,   // ответ: готов
    kCmdUpdateBusy   = 0x0445,   // ответ: идёт стирание флеша, опрашивать дальше
    kCmdUpdateData   = 0x8445,   // блок данных (по SPI, 256-байт пакеты)
    kCmdUpdateEnd    = 0x0446,   // завершение (UART)
    kCmdUpdateEndAck = 0x8446,   // ответ на завершение
};

} // namespace mcu
