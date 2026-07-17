#include "kernel/KControlProc.h"
#include "kernel/KControlINI.h"
#include "kernel/yxyDES2.h"
#include "sys/KSystemSet.h"

#include <QDate>
#include <QDir>
#include <QFile>
#include <QSettings>
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

// Off-device-параметризация DEVICE-корней (реф. USB-путь / живой эндоскоп).
QString g_importRoot;
QString g_curEndoSN;

// Имя секции истории по типу лицензии (реф. литералы @0x894790..).
const char *typeName(_KControlType t)
{
    switch (t) {
    case KCT_PROCESSOR_RELEASE: return "processor_release";
    case KCT_PROCESSOR_DELAY:   return "processor_delay";
    case KCT_IMPORT_ENDO:       return "import_endo";
    case KCT_ENDO_RELEASE:      return "endo_release";
    case KCT_ENDO_DELAY:        return "endo_delay";
    case KCT_IMPORT_PROCESSOR:  return "import_processor";
    default:                    return nullptr;
    }
}
} // namespace

void KControlProc::SetImportRoot(const QString &dir) { g_importRoot = dir; }
void KControlProc::SetCurEndoSN(const QString &sn)   { g_curEndoSN = sn; }

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

// ============================================================================
// Лицензирование (реф. — поверх KControlINI + DES + KSystemSet). DEVICE-корни
// параметризованы (SetImportRoot/SetCurEndoSN); реф. USB-gate GetMcFilenameList опущен.
// ============================================================================

QString KControlProc::McDirLicense() const
{
    // реф. KSystem::ImportPath() + "license/"; off-device — settable import-root.
    return g_importRoot + "license/";
}

bool KControlProc::IsLicensePathExist() const
{
    return QDir(McDirLicense()).exists();
}

QStringList KControlProc::GetMcFilenameList() const
{
    // реф. сначала GetUsbDevice()->IsUsbDisconnect() (DEVICE, опущено off-device), затем
    // QDir(McDirLicense()).entryInfoList("*.ini") → имена файлов.
    QStringList names;
    QDir dir(McDirLicense());
    if (!dir.exists())
        return names;
    for (const QFileInfo &fi : dir.entryInfoList(QStringList() << "*.ini", QDir::Files))
        names << fi.fileName();
    return names;
}

bool KControlProc::IsMd5sumUsed(QString md5, _KControlType type)
{
    const char *key = typeName(type);
    if (!key)
        return true;   // реф. type>=6 → true без файла
    QSettings s(KControlINI::HistoryLicenseRecord(), QSettings::IniFormat);
    QStringList used = s.value(key).toStringList();
    if (used.contains(md5, Qt::CaseSensitive))
        return true;                       // уже израсходован
    used.append(md5);
    s.setValue(key, used);                 // side-effect: помечаем как использованный
    return false;
}

int KControlProc::CheckLicense(QString sn, _KControlType type)
{
    if (!IsLicensePathExist())
        return 1;                          // нет каталога лицензий

    const QString fname = LicenseFileName(sn, type);
    if (!GetMcFilenameList().contains(fname, Qt::CaseSensitive))
        return 2;                          // нет файла лицензии

    QString full = McDirLicense() + fname;
    const QString plainPath = Cipher2Plain(full);
    if (plainPath.isEmpty())
        return 3;                          // расшифровка/запись не удалась

    QSettings s(plainPath, QSettings::IniFormat);
    const QString md5 = s.value("md5sum").toString();
    if (IsMd5sumUsed(md5, type))
        return 4;                          // md5 уже использован (и помечает)

    const QString fSN     = s.value("SN", "").toString();
    const QString fState  = s.value("ControlState", "").toString();
    const QString fProcSN = s.value("ProcessorSN", "").toString();
    const QString fEndoSN = s.value("EndoSN", "").toString();
    const QString machSN  = KSystemSet::GetInstance().GetProcessorSN();

    bool okFields = false;
    switch (type) {
    case KCT_PROCESSOR_RELEASE: okFields = (fSN == sn && fState == "release"); break;
    case KCT_PROCESSOR_DELAY:   okFields = (fSN == sn && fState == "delay");   break;
    case KCT_IMPORT_ENDO:       okFields = (fProcSN == sn);                    break;
    case KCT_ENDO_RELEASE:      okFields = (fSN == sn && fState == "release" && fProcSN == machSN); break;
    case KCT_ENDO_DELAY:        okFields = (fSN == sn && fState == "delay"   && fProcSN == machSN); break;
    case KCT_IMPORT_PROCESSOR:  okFields = (fEndoSN == sn && fProcSN == machSN); break;
    default: break;
    }
    return okFields ? 0 : 4;
}

QString KControlProc::GetDeadline() const
{
    // реф. — ОТДЕЛЬНО от KControlINI::GetDeadline: читает plain.ini, ключ "deadline", дефолт "".
    QSettings s(KControlINI::PlainINIpath(), QSettings::IniFormat);
    return s.value("deadline", "").toString();
}

int KControlProc::GetDelayTime() const
{
    QSettings s(KControlINI::PlainINIpath(), QSettings::IniFormat);
    return s.value("addNum", 0).toInt();
}

bool KControlProc::IsStartTimeMc() const { return KControlINI::IsStartTimeControl(); }
bool KControlProc::IsStartEndoMc() const { return KControlINI::IsStartEndoControl(); }

QString KControlProc::GetCurEndoSN() const
{
    // реф. GetEndoScope()->GetEndoInfo() (DEVICE, живой эндоскоп); off-device — settable.
    return g_curEndoSN;
}

bool KControlProc::IsOutofControl()
{
    if (IsStartTimeMc() && KControlINI::GetRemainDays() == 0)
        return true;                       // истёк лимит времени
    if (!IsStartEndoMc())
        return false;                      // endo-контроль выключен → не заблокирована
    const QString cur = GetCurEndoSN();
    if (KControlINI::GetMatchEndos().contains(cur, Qt::CaseSensitive))
        return false;                      // текущий эндоскоп разрешён
    return true;                           // endo-контроль вкл., текущий не в списке
}

// ============================================================================
// Машинный контроль: время (control.ini + даты) и эндоскопы (блочная запись).
// EEPROM-методы (GetEndoRemainTimes/GetEndoDeadline/GetMatchProcessorList/
// IsStartMatchProcessorCtrl/IsStartEndoUseTimeCtrl/IsEndoPowerOn) — DEVICE-BOUND, не реализованы.
// ============================================================================

void KControlProc::StartTimeMc(const _MC_Time *t)
{
    if (!t)
        return;   // реф. — тихий no-op, даже без лога
    // реф. логирует deadline и пишет структуру КАК ЕСТЬ. Флаг controlTime сам НЕ ставит.
    KControlINI::WriteMcTime(*t);
}

void KControlProc::StopTimeMc()
{
    _MC_Time t;
    t.controlTime = false;
    t.deadline    = "2099-01-01";   // дефолт KControlINI::GetDeadline, не пустая строка
    t.remainDays  = -1;             // sic: НЕ 0 — «разоружено», IsOutofControl сверяет ==0
    KControlINI::WriteMcTime(t);
}

void KControlProc::UpdateMcDays()
{
    if (!KControlINI::IsStartTimeControl())
        return;
    int remain = KControlINI::GetRemainDays();
    if (remain <= 0)
        return;                     // -1 от StopTimeMc так и не нормализуется в 0 (реф.)

    const QDate deadline = QDate::fromString(KControlINI::GetDeadline(), "yyyy-MM-dd");
    const int days = QDate::currentDate().daysTo(deadline);   // СИСТЕМНЫЕ ЧАСЫ
    if (remain > days)                                        // храповик: только вниз
        remain = (days >= 0) ? days : 0;                      // просрочка/битая дата → 0
    KControlINI::SetRemainDays(remain);
}

void KControlProc::StartEndoMc(QStringList endos)
{
    // Одна блочная запись: флаг и список не могут разойтись (реф. не через Set* по отдельности).
    _MC_Endo e;
    e.controlEndo = true;
    e.endos = endos;
    KControlINI::WriteMcEndo(e);    // реф. лога здесь НЕТ (асимметрия со StopEndoMc)
}

void KControlProc::StopEndoMc()
{
    _MC_Endo e;
    e.controlEndo = false;
    e.endos.clear();
    KControlINI::WriteMcEndo(e);
}

bool KControlProc::IsEndoMatch()
{
    // реф. дублирует это тело внутри IsOutofControl (не вызывает) — у нас так же.
    return KControlINI::GetMatchEndos().contains(GetCurEndoSN(), Qt::CaseSensitive);
}
