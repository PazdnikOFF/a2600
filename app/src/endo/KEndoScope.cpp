#include "endo/KEndoScope.h"

#include <cstring>

#include "endo/KEndoScopeTables.h"
#include "kernel/KSystemLog.h"
#include "sys/KAccount.h"
#include "sys/KEncStyle.h"

namespace {

// Реф.: 6 записей, точное равенство, регистрозависимо.
const QStringList &superfineList()
{
    static const QStringList l = {
        QStringLiteral("EUC-X20S"), QStringLiteral("ECH-110S"),
        QStringLiteral("ECH-X20S"), QStringLiteral("EUC-110S"),
        QStringLiteral("ENL-110S"), QStringLiteral("ENL-X20S"),
    };
    return l;
}

// Реф.: 4 записи, точное равенство, регистрозависимо.
const QStringList &channelList()
{
    static const QStringList l = {
        QStringLiteral("ENL-110S"), QStringLiteral("ENL-X20S"),
        QStringLiteral("ENL-110"),  QStringLiteral("ENL-X20"),
    };
    return l;
}

// Строка фиксированной ширины из EEPROM/CID: реф. использует strlen по полю
// без гарантии терминатора, поэтому ограничиваем максимумом поля.
QString fixedStr(const unsigned char *p, int maxLen)
{
    int n = 0;
    while (n < maxLen && p[n] != '\0')
        ++n;
    return QString::fromLatin1(reinterpret_cast<const char *>(p), n);
}

unsigned short rdU16(const unsigned char *p) { return (unsigned short)(p[0] | (p[1] << 8)); }
unsigned int   rdU32(const unsigned char *p)
{
    return unsigned(p[0]) | (unsigned(p[1]) << 8) | (unsigned(p[2]) << 16)
         | (unsigned(p[3]) << 24);
}
void wrU16(unsigned char *p, unsigned short v) { p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF; }
void wrU32(unsigned char *p, unsigned int v)
{
    p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF;
    p[2] = (v >> 16) & 0xFF; p[3] = (v >> 24) & 0xFF;
}

} // namespace

KEndoScope::KEndoScope(QObject *parent) : QObject(parent)
{
    m_pEndoInfo = new _EndoInfoStruct();
    m_pEepromData = new _EepromInfo();
}

KEndoScope::~KEndoScope()
{
    delete m_pEndoInfo;
    delete m_pEepromData;
}

KEndoScope *GetEndoScope()
{
    static KEndoScope inst;   // реф.: function-local static + __cxa_atexit
    return &inst;
}

// --- состояние --------------------------------------------------------------

bool KEndoScope::IsEndoReady() const { return m_endoScopeStatus == 4; }

bool KEndoScope::IsEndoOutOfControl() const
{
    return m_endoControlStatus != 0 && m_endoControlStatus != 5;
}

void KEndoScope::SetEndoScopeStatus(int v) { m_endoScopeStatus = v; }
void KEndoScope::SetEndoControlStatus(int v) { m_endoControlStatus = v; }

// --- принадлежность модели --------------------------------------------------

bool KEndoScope::IsSuperfineEndo(const QString &model)
{
    // Полярность выверена дизасмом: результат contains возвращается НАПРЯМУЮ.
    return superfineList().contains(model, Qt::CaseSensitive);
}

bool KEndoScope::IsEndoHasChannel(const QString &model)
{
    return channelList().contains(model, Qt::CaseSensitive);
}

bool KEndoScope::IsVideoCalReveral() const
{
    const QString &m = m_pEndoInfo->sModel;
    // Первые два — ПОДСТРОКА, третий — полное равенство. Всё регистрозависимо.
    return m.indexOf(QLatin1String("430"), 0, Qt::CaseSensitive) != -1
        || m.indexOf(QLatin1String("500"), 0, Qt::CaseSensitive) != -1
        || m == QLatin1String("ED-5GT");
}

bool KEndoScope::IsEndoModelHaveSuffix(const QString &model)
{
    // ⚠️ Имя лжёт: это поиск ПОДСТРОКИ "430", никаких суффиксов.
    return model.indexOf(QLatin1String("430"), 0, Qt::CaseSensitive) != -1;
}

bool KEndoScope::ISEndoScopeMatch(const QString &model, const QString &id) const
{
    // Привилегированный обход: роль > 2.
    if (int(KAccount::GetInstance().CurrentRole()) > 2)
        return true;
    if (model.isEmpty()) {
        LogPrintfEx(true, "[APP][E]: ", "Endo Check: the endo model is empty.\n");
        return false;
    }
    if (!KEncStyle().IsScopeValid(model)) {
        LogPrintfEx(true, "[APP][E]: ",
                    "Endo Check: the endo model is not in the list.\n");
        return false;
    }
    if (id.isEmpty()) {
        LogPrintfEx(true, "[APP][E]: ", "Endo Check: the endo ID is empty.\n");
        return false;
    }
    return true;
}

// --- серии ------------------------------------------------------------------

const QMap<QString, QString> &KEndoScope::GetEndoSeriesMap() { return kEndoSeriesMap; }
const QMap<QString, QString> &KEndoScope::GetEndoSeriesMapWuHan()
{
    return kEndoSeriesMapWuHan;
}

// --- разбор CID -------------------------------------------------------------

unsigned char KEndoScope::makeCRC4Endo(const unsigned char *pData)
{
    // ⚠️ ВОПРЕКИ ИМЕНИ это НЕ CRC: 8-битный бегущий XOR, seed 0, без полинома,
    // по байтам 0x00..0x7e (127 байт). Сверяется с байтом 0x7f.
    unsigned char x = 0;
    for (int i = 0; i <= 0x7e; ++i)
        x ^= pData[i];
    return x;
}

void KEndoScope::ExtractEndoinfoFromdata(const unsigned char *pData)
{
    _EndoInfoStruct *p = m_pEndoInfo;
    // ⚠️ КВИРК: модель ОЧИЩАЕТСЯ и здесь НЕ ЗАПОЛНЯЕТСЯ (разбирается в
    // локальную переменную и теряется). Ставит её только CheckEndoInfo.
    p->sModel.clear();
    // ⚠️ .trimmed() — ТОЛЬКО для первых 16 байт; остальные строки НЕ обрезаются.
    (void)fixedStr(pData + 0x00, 0x10).trimmed();

    p->sEndoID         = fixedStr(pData + 0x10, 0x0c);
    p->b10 = pData[0x1c];
    p->b11 = pData[0x1d];
    p->b12 = pData[0x1e];
    p->b13 = pData[0x1f];
    p->u30 = rdU16(pData + 0x20);
    p->sWarrantyDate   = fixedStr(pData + 0x22, 0x08);
    p->sServerContract = fixedStr(pData + 0x2a, 0x10);
    p->sComment        = fixedStr(pData + 0x3a, 0x3c);
    p->nGlassType      = pData[0x76];
    // Байты 0x77..0x7e не используются.
}

// --- разбор EEPROM ----------------------------------------------------------

void KEndoScope::ExtractFixParam(const unsigned char *pData, unsigned char /*nUnused*/)
{
    _EepromInfo *e = m_pEepromData;
    const unsigned char f = e->fixFlags;

    if (f & 0x01) {
        e->centorX = int(rdU32(pData + 0x00));
        e->centorY = int(rdU32(pData + 0x04));
        // Реф.: значения > 0x20 считаются мусором и заменяются на 16.
        if (e->centorX > 0x20) e->centorX = 16;
        if (e->centorY > 0x20) e->centorY = 16;
        emit EepromParamUpdated(1);
    }
    if (f & 0x02) {
        e->wbRed  = rdU16(pData + 0x08);
        e->wbBlue = rdU16(pData + 0x0c);
        emit EepromParamUpdated(0);
    }
    if (f & 0x08) {
        e->octangleCut = rdU32(pData + 0x12);     // ⚠️ hi16 = Y, lo16 = X
        const unsigned hi = (e->octangleCut >> 16) & 0xFFFF;
        const unsigned lo = e->octangleCut & 0xFFFF;
        if (hi > 960 || lo < 540)
            e->octangleCut = 0x005A005A;
        emit EepromParamUpdated(3);
    }
    if (f & 0x10)
        e->usedCount = rdU16(pData + 0x16);
    // Читается ВСЕГДА, без флага.
    e->matchProcMask = rdU16(pData + 0x2f);
    if (f & 0x40)
        e->remainUseTimes = rdU16(pData + 0x2d);
}

void KEndoScope::ExtractExtraFixParam(const unsigned char *pData, unsigned char /*nUnused*/)
{
    _EepromInfo *e = m_pEepromData;
    const unsigned int f = e->extraFlags;

    if (f & 0x01) {
        e->roundCut = (short)(rdU16(pData + 0x00));
        emit EepromParamUpdated(3);
    }
    if (f & 0x02) {
        e->videoCapX = int(rdU32(pData + 0x02));
        e->videoCapY = int(rdU32(pData + 0x06));
        emit EepromParamUpdated(2);
    }
    // 0x0a..0x0f — неиспользуемый разрыв.
    if (f & 0x08) {
        e->deadline = fixedStr(pData + 0x10, 8).left(8);
        // ⚠️ Для deadline сигнал НЕ шлётся (в отличие от двух веток выше).
    }
}

void KEndoScope::ExactMatchProcessorEepromData(const unsigned char *pData,
                                               unsigned char /*nUnused*/, int nBufLen)
{
    _EepromInfo *e = m_pEepromData;
    // ⚠️ КВИРК: очистка ДО валидации — мусор затирает хороший список без отката.
    e->matchProcList.clear();
    if (pData[0] != 0xAA)
        return;
    int n = pData[1];
    // ⚠️ В ОРИГИНАЛЕ n НЕ ОГРАНИЧИВАЕТСЯ, а переданная длина игнорируется ⇒
    // чтение до ~3 КБ за границей 64-байтовой страницы. Мы ограничиваем явно:
    // воспроизводить порчу памяти нельзя, поэтому отступление помечено.
    const int maxRec = (nBufLen - 2) / 12;
    if (n > maxRec)
        n = maxRec;
    for (int i = 0; i < n; ++i)
        e->matchProcList << fixedStr(pData + 2 + i * 12, 12);
}

void KEndoScope::SaveFixParam()
{
    unsigned char *b = m_eepromSaveData;
    memset(b, 0, sizeof(m_eepromSaveData));
    const _EepromInfo *e = m_pEepromData;

    b[0x00] = e->fixFlags;
    wrU32(b + 0x01, (unsigned)(e->centorX));
    wrU32(b + 0x05, (unsigned)(e->centorY));
    wrU16(b + 0x09, e->wbRed);
    wrU16(b + 0x0d, e->wbBlue);
    wrU32(b + 0x13, e->octangleCut);
    wrU16(b + 0x17, e->usedCount);
    wrU32(b + 0x20, e->extraFlags);
    wrU16(b + 0x24, (unsigned short)(e->roundCut));
    wrU32(b + 0x26, (unsigned)(e->videoCapX));
    wrU32(b + 0x2a, (unsigned)(e->videoCapY));
    wrU16(b + 0x2e, e->remainUseTimes);
    wrU16(b + 0x30, e->matchProcMask);
    const QByteArray d = e->deadline.toLatin1();
    for (int i = 0; i < 8 && i < d.size(); ++i)
        b[0x34 + i] = (unsigned char)(d[i]);
    // Дыры 0x0b-0x0c, 0x0f-0x12, 0x19-0x1f, 0x32-0x33, 0x3c-0x3f остаются нулями.
    emit SaveEndoEepromData(511);
}

void KEndoScope::SaveMatchProcessor2Eeprom(const QStringList &lst)
{
    unsigned char *b = m_eepromSaveData;
    memset(b, 0, sizeof(m_eepromSaveData));
    if (!lst.isEmpty()) {
        b[0] = 0xAA;
        // ⚠️ КВИРК: в байт длины пишется ПОЛНЫЙ размер списка, а записей
        // сохраняется не более 5 — страница может заявлять 7 при пяти данных.
        b[1] = (unsigned char)(lst.size());
        const int n = qMin(lst.size(), 5);
        for (int i = 0; i < n; ++i) {
            const QByteArray s = lst[i].toLatin1();
            for (int j = 0; j < 12 && j < s.size(); ++j)
                b[2 + i * 12 + j] = (unsigned char)(s[j]);
        }
    }
    emit SaveEndoEepromData(508);
}

void KEndoScope::ResetEepromData()
{
    _EepromInfo *e = m_pEepromData;
    e->centorX = 16;   e->centorY = 16;
    e->videoCapX = 0;  e->videoCapY = 0;
    const KEncStyle enc;
    e->octangleCut = (unsigned)(enc.getScopeDefaultOctangleCut(QString()));
    e->roundCut = (short)(enc.getScopeDefaultRoundCut(QString()));
    e->wbRed = 18000;
    e->wbBlue = 11800;
    e->usedCount = 0xFFFF;
    e->remainUseTimes = 0xFFFF;
    e->matchProcMask = 0xFFFF;
    e->deadline = QString();
    e->matchProcList.clear();
    // ⚠️ Черновую страницу (m_eepromSaveData) реф. НЕ ТРОГАЕТ.
}

void KEndoScope::AddEndoUsedCount()
{
    _EepromInfo *e = m_pEepromData;
    // ⚠️ Декремент выполняется ДО раннего выхода — при usedCount == 0xFFFF
    // он теряется, потому что SaveFixParam не вызывается.
    --e->remainUseTimes;
    if (e->usedCount == 0xFFFF)
        return;
    // ⚠️ 0xFFFE инкрементится ровно в «недействительный» сентинел 0xFFFF.
    ++e->usedCount;
    SaveFixParam();
}

// --- геометрия --------------------------------------------------------------

void KEndoScope::GetCentorPointStart(int &x, int &y) const
{
    switch (GetEndoFirmwareType()) {
    case 0: x = 8;   y = 8;  break;
    case 1:
        if (GetRotateType() == 2) { x = 168; y = 23; }
        else                      { x = 167; y = 24; }
        break;
    case 3: x = 167; y = 23; break;
    case 5: x = 328; y = 72; break;
    case 8: x = 329; y = 73; break;
    default: x = 0; y = 0; break;   // включая 2 и 4
    }
}

int KEndoScope::GetCentorPointXRange() const
{
    int x = 0, y = 0;
    GetCentorPointStart(x, y);
    return x / 2;
}

int KEndoScope::GetCentorPointYRange() const
{
    int x = 0, y = 0;
    GetCentorPointStart(x, y);
    return y / 2;
}

// --- делегаты в KEncStyle ---------------------------------------------------

int KEndoScope::GetEndoType() const
{
    return KEncStyle().GetEndoType(m_pEndoInfo->sModel);
}
int KEndoScope::GetSensorType() const
{
    return KEncStyle().GetEndoSensorType(m_pEndoInfo->sModel);
}
int KEndoScope::GetEndoFirmwareType() const
{
    return KEncStyle().GetFirmwareType(m_pEndoInfo->sModel);
}
int KEndoScope::GetEndoShapeType() const
{
    return KEncStyle().GetEndoShapeType(m_pEndoInfo->sModel);
}

int KEndoScope::GetRotateType() const
{
    return m_pEndoInfo ? m_pEndoInfo->nRotateType : 0;
}

int KEndoScope::GetEndoGlassType() const
{
    return m_pEndoInfo ? m_pEndoInfo->nGlassType : 3;   // ⚠️ дефолт 3
}

float KEndoScope::GetZoomRatio() const
{
    // ⚠️ В реф. null-проверка и запасное 1.0f МЕРТВЫ: сразу после них идёт
    // безусловное разыменование sModel.
    float ratio = m_pEndoInfo->fZoomRatio;
    const QString &model = m_pEndoInfo->sModel;
    // Реф. условие ПОЛНОЕ: IsSuperfineEndo(model) && KSystemSet::GetCornerCutSize(model) == 1.
    // ⚠️ У нас KSystemSet::GetCornerCutSize ЕЩЁ НЕ РЕАЛИЗОВАН, поэтому вторая
    // половина условия ОПУЩЕНА — это ОТСТУПЛЕНИЕ от оригинала, а не квирк:
    // при superfine-модели с иным размером обрезки углов реф. вернул бы
    // fZoomRatio, а мы вернём 1.36f. Дореализовать вместе с KSystemSet.
    if (IsSuperfineEndo(model))
        ratio = 1.36f;
    return ratio;
}
