#include "kernel/KControlProc.h"
#include "kernel/KControlINI.h"
#include "kernel/yxyDES2.h"

#include <QDate>
#include <QFile>
#include <QTextStream>

#include <cstring>
#include <new>
#include <vector>

namespace {
// Ключ DES машинного контроля (реф. собирает инлайн-иммедиатами: mov 0x585a / movk 0x7559,16 /
// movk 0x6f69,32 / movk 0x3231,48 → LE-байты 5A 58 59 75 69 6F 31 32).
const char DES_KEY[8] = { 'Z','X','Y','u','i','o','1','2' };

const int HEX_BUF  = 8192;    // реф. sp-буфер под hex-строку
const int SCRATCH  = 16384;   // реф. два 16КБ скретча в ConvertOtherFormat2Ciphertext
} // namespace

int KControlProc::ConvertOtherFormat2Ciphertext(yxyDES2 *des, char *out, char *inHex)
{
    if (!des || !out || !inHex)
        return -1;

    const std::size_t len = std::strlen(inHex);
    // реф.: len округляется ВВЕРХ до кратной 4 hex-символам, затем ×16 бит на 4 символа.
    const unsigned int nBits = static_cast<unsigned int>(((len + 3) / 4) * 16);

    std::vector<char> hexCopy(SCRATCH, 0);   // Hex2Bits ПОРТИТ вход → работаем по копии
    std::vector<char> bits(SCRATCH, 0);
    if (len)
        std::memcpy(hexCopy.data(), inHex, len);   // реф. NUL не дописывает

    des->Hex2Bits(hexCopy.data(), bits.data(), nBits);
    des->Bits2Bytes(out, bits.data(), nBits);
    return static_cast<int>(nBits >> 3);
}

QString KControlProc::DecryptionStr(QString &cipherHex)
{
    QString ret;
    std::vector<char> szHex(HEX_BUF, 0);
    const QByteArray ba = cipherHex.toLatin1();
    if (!ba.isEmpty()) {
        const int n = qMin(ba.size(), HEX_BUF - 1);
        std::memcpy(szHex.data(), ba.constData(), static_cast<std::size_t>(n));
    }

    yxyDES2 *des = new (std::nothrow) yxyDES2;
    if (!des)
        return ret;

    char key[32] = { 0 };                       // реф. — 32-байтный нулевой буфер, ключ в начале
    std::memcpy(key, DES_KEY, sizeof(DES_KEY));
    des->InitializeKey(key, 0);                 // слот расписания 0

    std::vector<char> bytes(HEX_BUF, 0);
    const int n = ConvertOtherFormat2Ciphertext(des, bytes.data(), szHex.data());
    if (n != -1) {
        des->DecryptAnyLength(bytes.data(), static_cast<unsigned int>(n), 0);
        const char *pt = des->GetPlainAnyLength();
        ret = QString::fromLatin1(pt, pt ? static_cast<int>(std::strlen(pt)) : 0);
    }
    delete des;
    return ret;
}

QString KControlProc::Cipher2Plain(QString &cipherFilePath)
{
    QFile f(cipherFilePath);
    if (!f.exists())                                    // реф. — лог "input file … isn't exist"
        return QString("");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) // реф. — лог "open file failure"
        return QString("");
    QTextStream ts(&f);
    QString cipher = ts.readAll();
    const QString plain = DecryptionStr(cipher);        // единственный крипто-шаг
    f.close();

    const QString outPath = KControlINI::PlainINIpath();
    QFile out(outPath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text))
        return QString("");
    QTextStream os(&out);
    os << plain;
    out.flush();
    out.close();
    return outPath;                                     // ПУТЬ файла, не плейнтекст
}

QString KControlProc::LicenseFileName(QString sn, _KControlType type)
{
    switch (type) {
    case KCT_PROCESSOR_RELEASE:
    case KCT_ENDO_RELEASE:
        return sn + "_release.ini";
    case KCT_PROCESSOR_DELAY:
    case KCT_ENDO_DELAY:
        return sn + "_delay.ini";
    case KCT_IMPORT_ENDO:
    case KCT_IMPORT_PROCESSOR:
        return sn + "_import.ini";
    default:
        return QString();          // >=6 → пусто
    }
}

void KControlProc::SystemDate2MCDate(QString date)
{
    if (!KControlINI::IsStartTimeControl())
        return;
    const int remain = KControlINI::GetRemainDays();
    if (remain <= 0)
        return;
    const QDate d = QDate::fromString(date, "yyyy-MM-dd");
    const QDate dl = d.addDays(remain);
    KControlINI::SetDeadline(dl.toString("yyyy-MM-dd"));   // невалидная дата → пустая строка
}
