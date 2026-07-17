#pragma once

// Реализация DES (реф. yxyDES2, X-2600, `yxyDES2.cpp`). STANDALONE: без vtable, без Q_OBJECT,
// без device-зависимостей — чистый алгоритм. Все таблицы (PC-1/PC-2/IP/IP-1/E/P/S1..S8/сдвиги)
// сверены с бинарником (@0x8942a8..0x8945b0) и совпадают со СТАНДАРТНЫМ DES байт-в-байт.
//
// «DES2» = ДВА независимых расписания ключей в одном объекте: параметр `uint` у InitializeKey/
// EncryptData/DecryptData — это НЕ длина, а НОМЕР СЛОТА расписания (0 или 1). Двойного/тройного
// применения DES нет — это обычный single DES, ECB, без паддинга на уровне блока.
//
// Биты хранятся ПО ОДНОМУ НА char, порядок MSB-first. Результаты кладутся в поля-члены
// (аргумент `data` НЕ модифицируется), геттеры отдают указатели внутрь объекта.
class yxyDES2
{
public:
    yxyDES2();
    ~yxyDES2() = default;

    // Ключ — РОВНО 8 байт; keySel ∈ {0,1} — слот расписания.
    void InitializeKey(char *key, unsigned int keySel);

    // ОДИН 8-байтный блок → m_CipherBits/m_CipherBytes (data не меняется).
    void EncryptData(char *data8, unsigned int keySel);
    void DecryptData(char *data8, unsigned int keySel);   // раунды 15..0

    // Произвольная длина (реф. квирки сохранены, см. .cpp):
    //  * len==8  → ровно 8 байт;
    //  * len<8   → нуль-паддинг до 8;
    //  * len>8   → ВСЕГДА один ЛИШНИЙ блок (при len%8==0 это шифр восьми нулей);
    //              длина выхода = (len/8 + 1)*8.
    void EncryptAnyLength(char *data, unsigned int len, unsigned int keySel);
    //  * длина выхода РОВНО len; хвостовой блок только при len%8!=0 (асимметрия с encrypt).
    void DecryptAnyLength(char *data, unsigned int len, unsigned int keySel);

    const char *GetCiphertextInBinary();   // 64 символа '0'/'1' + NUL
    const char *GetCiphertextInHex();      // 16 hex-символов ВЕРХНЕГО регистра + NUL
    const char *GetCiphertextInBytes();    // 8 байт
    const char *GetPlaintext();            // 8 байт + NUL
    const char *GetCipherAnyLength() { return m_CipherAny; }
    const char *GetPlainAnyLength() { return m_PlainAny; }

    // Низкоуровневые примитивы (в реф. — публичные методы класса).
    void Bytes2Bits(char *In, char *Out, unsigned int nBits);
    void Bits2Bytes(char *Out, char *In, unsigned int nBits);
    void Int2Bits(unsigned int v, char *Out);            // 4 бита, MSB-first
    void Bits2Hex(char *Out, char *In, unsigned int nBits);   // UPPERCASE, БЕЗ NUL
    void Hex2Bits(char *In, char *Out, unsigned int nBits);   // ВНИМАНИЕ: портит In на месте
    void XOR(char *a, char *b, unsigned int len, char *out);
    void InitialPermuteData(char *In, char *Out);        // IP
    void ExpansionR(char *In, char *Out);                // E
    void PermutationP(char *In, char *Out);              // P
    void CompressFuncS(char *In48, char *Out32);         // S-боксы
    void CreateSubKey(char *k56, unsigned int keySel);   // PC-1 → C/D → сдвиги → PC-2
    void FunctionF(char *L, char *R, unsigned int round, unsigned int keySel);   // раунд Фейстеля

private:
    char m_SubKey[2][16][48];      // +0x0000 — 2 расписания × 16 раундов × 48 бит
    char m_CipherBits[64];         // +0x0600
    char m_PlainBits[64];          // +0x0640
    char m_CipherBytes[8];         // +0x0680
    char m_PlainBytes[8];          // +0x0688
    char m_szCipherBinary[65];     // +0x0690
    char m_szCipherHex[17];        // +0x06D1
    char m_szPlain[9];             // +0x06E2
    char m_CipherAny[8192];        // +0x06EB
    char m_PlainAny[8192];         // +0x26EB
};   // sizeof = 0x46EB = 18155 (align 1) — как в реф.
