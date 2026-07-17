#include "kernel/yxyDES2.h"

#include <cstring>

namespace {

// Все таблицы сверены с бинарником (@0x8942a8..0x8945b0) — СТАНДАРТНЫЙ DES, 1-based.
const unsigned char SHIFT[16] = { 1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1 };           // @0x8942a8

const unsigned char PC2[48] = {                                                 // @0x8942b8
    14,17,11,24, 1, 5, 3,28,15, 6,21,10,
    23,19,12, 4,26, 8,16, 7,27,20,13, 2,
    41,52,31,37,47,55,30,40,51,45,33,48,
    44,49,39,56,34,53,46,42,50,36,29,32 };

const unsigned char IP[64] = {                                                  // @0x8942e8
    58,50,42,34,26,18,10, 2,60,52,44,36,28,20,12, 4,
    62,54,46,38,30,22,14, 6,64,56,48,40,32,24,16, 8,
    57,49,41,33,25,17, 9, 1,59,51,43,35,27,19,11, 3,
    61,53,45,37,29,21,13, 5,63,55,47,39,31,23,15, 7 };

const unsigned char E[48] = {                                                   // @0x894328
    32, 1, 2, 3, 4, 5, 4, 5, 6, 7, 8, 9,
     8, 9,10,11,12,13,12,13,14,15,16,17,
    16,17,18,19,20,21,20,21,22,23,24,25,
    24,25,26,27,28,29,28,29,30,31,32, 1 };

const unsigned char P[32] = {                                                   // @0x894358
    16, 7,20,21,29,12,28,17, 1,15,23,26, 5,18,31,10,
     2, 8,24,14,32,27, 3, 9,19,13,30, 6,22,11, 4,25 };

const unsigned char PC1[56] = {                                                 // @0x894378
    57,49,41,33,25,17, 9, 1,58,50,42,34,26,18,
    10, 2,59,51,43,35,27,19,11, 3,60,52,44,36,
    63,55,47,39,31,23,15, 7,62,54,46,38,30,22,
    14, 6,61,53,45,37,29,21,13, 5,28,20,12, 4 };

const unsigned char IPINV[64] = {                                               // @0x8945b0
    40, 8,48,16,56,24,64,32,39, 7,47,15,55,23,63,31,
    38, 6,46,14,54,22,62,30,37, 5,45,13,53,21,61,29,
    36, 4,44,12,52,20,60,28,35, 3,43,11,51,19,59,27,
    34, 2,42,10,50,18,58,26,33, 1,41, 9,49,17,57,25 };

// S1..S8, непрерывно uint8 S[8][64] (@0x8943b0), индексация S[box][row*16+col].
const unsigned char SBOX[8][64] = {
  { 14, 4,13, 1, 2,15,11, 8, 3,10, 6,12, 5, 9, 0, 7,
     0,15, 7, 4,14, 2,13, 1,10, 6,12,11, 9, 5, 3, 8,
     4, 1,14, 8,13, 6, 2,11,15,12, 9, 7, 3,10, 5, 0,
    15,12, 8, 2, 4, 9, 1, 7, 5,11, 3,14,10, 0, 6,13 },
  { 15, 1, 8,14, 6,11, 3, 4, 9, 7, 2,13,12, 0, 5,10,
     3,13, 4, 7,15, 2, 8,14,12, 0, 1,10, 6, 9,11, 5,
     0,14, 7,11,10, 4,13, 1, 5, 8,12, 6, 9, 3, 2,15,
    13, 8,10, 1, 3,15, 4, 2,11, 6, 7,12, 0, 5,14, 9 },
  { 10, 0, 9,14, 6, 3,15, 5, 1,13,12, 7,11, 4, 2, 8,
    13, 7, 0, 9, 3, 4, 6,10, 2, 8, 5,14,12,11,15, 1,
    13, 6, 4, 9, 8,15, 3, 0,11, 1, 2,12, 5,10,14, 7,
     1,10,13, 0, 6, 9, 8, 7, 4,15,14, 3,11, 5, 2,12 },
  {  7,13,14, 3, 0, 6, 9,10, 1, 2, 8, 5,11,12, 4,15,
    13, 8,11, 5, 6,15, 0, 3, 4, 7, 2,12, 1,10,14, 9,
    10, 6, 9, 0,12,11, 7,13,15, 1, 3,14, 5, 2, 8, 4,
     3,15, 0, 6,10, 1,13, 8, 9, 4, 5,11,12, 7, 2,14 },
  {  2,12, 4, 1, 7,10,11, 6, 8, 5, 3,15,13, 0,14, 9,
    14,11, 2,12, 4, 7,13, 1, 5, 0,15,10, 3, 9, 8, 6,
     4, 2, 1,11,10,13, 7, 8,15, 9,12, 5, 6, 3, 0,14,
    11, 8,12, 7, 1,14, 2,13, 6,15, 0, 9,10, 4, 5, 3 },
  { 12, 1,10,15, 9, 2, 6, 8, 0,13, 3, 4,14, 7, 5,11,
    10,15, 4, 2, 7,12, 9, 5, 6, 1,13,14, 0,11, 3, 8,
     9,14,15, 5, 2, 8,12, 3, 7, 0, 4,10, 1,13,11, 6,
     4, 3, 2,12, 9, 5,15,10,11,14, 1, 7, 6, 0, 8,13 },
  {  4,11, 2,14,15, 0, 8,13, 3,12, 9, 7, 5,10, 6, 1,
    13, 0,11, 7, 4, 9, 1,10,14, 3, 5,12, 2,15, 8, 6,
     1, 4,11,13,12, 3, 7,14,10,15, 6, 8, 0, 5, 9, 2,
     6,11,13, 8, 1, 4,10, 7, 9, 5, 0,15,14, 2, 3,12 },
  { 13, 2, 8, 4, 6,15,11, 1,10, 9, 3,14, 5, 0,12, 7,
     1,15,13, 8,10, 3, 7, 4,12, 5, 6,11, 0,14, 9, 2,
     7,11, 4, 1, 9,12,14, 2, 0, 6,10,13,15, 3, 5, 8,
     2, 1,14, 7, 4,10, 8,13,15,12, 9, 0, 3, 5, 6,11 }
};

} // namespace

yxyDES2::yxyDES2()
{
    // реф. ctor обнуляет весь объект 0x0000..0x46EB.
    std::memset(this, 0, sizeof(yxyDES2));
}

void yxyDES2::Bytes2Bits(char *In, char *Out, unsigned int nBits)
{
    for (unsigned int i = 0; i < nBits; ++i)
        Out[i] = (In[i >> 3] >> (7 - (i & 7))) & 1;   // MSB-first
}

void yxyDES2::Bits2Bytes(char *Out, char *In, unsigned int nBits)
{
    std::memset(Out, 0, nBits / 8);
    for (unsigned int i = 0; i < nBits; ++i)
        Out[i >> 3] = static_cast<char>(Out[i >> 3] | (In[i] << (7 - (i & 7))));
}

void yxyDES2::Int2Bits(unsigned int v, char *Out)
{
    for (unsigned int i = 0; i < 4; ++i)
        Out[i] = static_cast<char>((v >> (3 - i)) & 1);
}

void yxyDES2::Bits2Hex(char *Out, char *In, unsigned int nBits)
{
    std::memset(Out, 0, nBits / 4);
    for (unsigned int i = 0; i < nBits; ++i)
        Out[i >> 2] = static_cast<char>(Out[i >> 2] + (In[i] << (3 - (i & 3))));
    for (unsigned int j = 0; j < nBits / 4; ++j)
        Out[j] = static_cast<char>(Out[j] + (Out[j] > 9 ? 55 : 48));   // ВЕРХНИЙ регистр
}

void yxyDES2::Hex2Bits(char *In, char *Out, unsigned int nBits)
{
    std::memset(Out, 0, nBits);
    for (unsigned int j = 0; j < nBits / 4; ++j)   // реф. ПОРТИТ In на месте
        In[j] = static_cast<char>(In[j] - (In[j] > 0x40 ? 55 : 48));
    for (unsigned int i = 0; i < nBits; ++i)
        Out[i] = (In[i >> 2] >> (3 - (i & 3))) & 1;
}

void yxyDES2::XOR(char *a, char *b, unsigned int len, char *out)
{
    for (unsigned int i = 0; i < len; ++i)
        out[i] = a[i] ^ b[i];
}

void yxyDES2::InitialPermuteData(char *In, char *Out)
{
    for (int i = 0; i < 64; ++i) Out[i] = In[IP[i] - 1];
}

void yxyDES2::ExpansionR(char *In, char *Out)
{
    for (int i = 0; i < 48; ++i) Out[i] = In[E[i] - 1];
}

void yxyDES2::PermutationP(char *In, char *Out)
{
    for (int i = 0; i < 32; ++i) Out[i] = In[P[i] - 1];
}

void yxyDES2::CompressFuncS(char *In48, char *Out32)
{
    for (int j = 0; j < 8; ++j) {
        const char *b = In48 + 6 * j;
        const int row = b[0] * 2 + b[5];
        const int col = b[1] * 8 + b[2] * 4 + b[3] * 2 + b[4];
        Int2Bits(SBOX[j][row * 16 + col], Out32 + 4 * j);
    }
}

void yxyDES2::CreateSubKey(char *k56, unsigned int keySel)
{
    char C[28], D[28], tC[28], tD[28], CD[56];
    std::memcpy(C, k56, 28);
    std::memcpy(D, k56 + 28, 28);
    for (int i = 0; i < 16; ++i) {
        const int s = SHIFT[i];
        std::memcpy(tC, C + s, 28 - s); std::memcpy(tC + 28 - s, C, s);   // циклический сдвиг влево
        std::memcpy(tD, D + s, 28 - s); std::memcpy(tD + 28 - s, D, s);
        std::memcpy(CD, tC, 28); std::memcpy(CD + 28, tD, 28);
        for (int j = 0; j < 48; ++j)
            m_SubKey[keySel][i][j] = CD[PC2[j] - 1];
        std::memcpy(C, tC, 28); std::memcpy(D, tD, 28);
    }
}

void yxyDES2::InitializeKey(char *key, unsigned int keySel)
{
    char b64[64], k56[56];
    Bytes2Bits(key, b64, 64);                 // ключ — РОВНО 8 байт
    for (int i = 0; i < 56; ++i) k56[i] = b64[PC1[i] - 1];
    CreateSubKey(k56, keySel);
}

void yxyDES2::FunctionF(char *L, char *R, unsigned int round, unsigned int keySel)
{
    char eR[48], x[48], s[32], p[32], nR[32];
    ExpansionR(R, eR);
    XOR(eR, m_SubKey[keySel][round], 48, x);
    CompressFuncS(x, s);
    PermutationP(s, p);
    XOR(p, L, 32, nR);
    std::memcpy(L, R, 32);    // L = R_old
    std::memcpy(R, nR, 32);   // R = L_old ^ f(R_old, K)
}

void yxyDES2::EncryptData(char *data8, unsigned int keySel)
{
    char bits[64], ip[64], L[32], R[32], t[64];
    Bytes2Bits(data8, bits, 64);
    InitialPermuteData(bits, ip);
    std::memcpy(L, ip, 32); std::memcpy(R, ip + 32, 32);
    for (unsigned int i = 0; i < 16; ++i) FunctionF(L, R, i, keySel);   // вперёд 0..15
    std::memcpy(t, R, 32); std::memcpy(t + 32, L, 32);                  // финальный swap
    for (int i = 0; i < 64; ++i) m_CipherBits[i] = t[IPINV[i] - 1];
    Bits2Bytes(m_CipherBytes, m_CipherBits, 64);
}

void yxyDES2::DecryptData(char *data8, unsigned int keySel)
{
    char bits[64], ip[64], L[32], R[32], t[64];
    Bytes2Bits(data8, bits, 64);
    InitialPermuteData(bits, ip);
    std::memcpy(L, ip, 32); std::memcpy(R, ip + 32, 32);
    for (int i = 15; i >= 0; --i) FunctionF(L, R, static_cast<unsigned int>(i), keySel);  // назад
    std::memcpy(t, R, 32); std::memcpy(t + 32, L, 32);
    for (int i = 0; i < 64; ++i) m_PlainBits[i] = t[IPINV[i] - 1];
    Bits2Bytes(m_PlainBytes, m_PlainBits, 64);
}

const char *yxyDES2::GetCiphertextInBinary()
{
    for (int i = 0; i < 64; ++i) m_szCipherBinary[i] = static_cast<char>(m_CipherBits[i] + '0');
    m_szCipherBinary[64] = 0;
    return m_szCipherBinary;
}

const char *yxyDES2::GetCiphertextInHex()
{
    Bits2Hex(m_szCipherHex, m_CipherBits, 64);
    m_szCipherHex[16] = 0;
    return m_szCipherHex;
}

const char *yxyDES2::GetCiphertextInBytes() { return m_CipherBytes; }

const char *yxyDES2::GetPlaintext()
{
    std::memcpy(m_szPlain, m_PlainBytes, 8);
    m_szPlain[8] = 0;
    return m_szPlain;
}

void yxyDES2::EncryptAnyLength(char *data, unsigned int len, unsigned int keySel)
{
    // реф. квирки сохранены: len==8 — спецслучай (ровно 8 байт); len>8 — ВСЕГДА лишний блок.
    if (len == 8) {
        EncryptData(data, keySel);
        std::memcpy(m_CipherAny, m_CipherBytes, 8);
        m_CipherAny[8] = 0;
    } else if (len < 8) {
        char t[8]; std::memset(t, 0, 8); std::memcpy(t, data, len);   // нуль-паддинг
        EncryptData(t, keySel);
        std::memcpy(m_CipherAny, m_CipherBytes, 8);
        m_CipherAny[8] = 0;
    } else {
        const unsigned int n = len / 8;
        for (unsigned int i = 0; i < n; ++i) {
            char t[8]; std::memcpy(t, data + 8 * i, 8);
            EncryptData(t, keySel);
            std::memcpy(m_CipherAny + 8 * i, m_CipherBytes, 8);
        }
        char t[8]; std::memset(t, 0, 8);
        std::memcpy(t, data + n * 8, len % 8);   // ВСЕГДА, даже при len%8==0 (шифр нулей)
        EncryptData(t, keySel);
        std::memcpy(m_CipherAny + n * 8, m_CipherBytes, 8);
        m_CipherAny[(n + 1) * 8] = 0;
    }
}

void yxyDES2::DecryptAnyLength(char *data, unsigned int len, unsigned int keySel)
{
    // Асимметрия с encrypt (реф.): длина выхода РОВНО len; хвост только при len%8!=0.
    const unsigned int n = len / 8;
    for (unsigned int i = 0; i < n; ++i) {
        char t[8]; std::memcpy(t, data + 8 * i, 8);
        DecryptData(t, keySel);
        std::memcpy(m_PlainAny + 8 * i, m_PlainBytes, 8);
    }
    if (len % 8 != 0) {
        char t[8]; std::memcpy(t, data + n * 8, 8);   // реф. читает ПОЛНЫЕ 8 байт (over-read)
        DecryptData(t, keySel);
        std::memcpy(m_PlainAny + n * 8, m_PlainBytes, len % 8);
    }
    m_PlainAny[len] = 0;
}
