#include "endo/KCamera.h"

#include <cstring>

#include "endo/KCameraTables.h"
#include "kernel/KSystemLog.h"
#include "sys/KAccount.h"
#include "sys/KEncStyle.h"

namespace {

// Реф.: поле копируется в 128-байтовый нулевой буфер и превращается в QString
// по strlen ⇒ строки NUL-ТЕРМИНИРОВАННЫЕ, а не фиксированной ширины.
QString cidStr(const unsigned char *p, int len)
{
    char buf[128] = {};
    if (len > 127)
        len = 127;
    memcpy(buf, p, size_t(len));
    return QString::fromUtf8(buf, int(strlen(buf)));
}

int rdI32(const unsigned char *p)
{
    return int((unsigned)(p[0]) | ((unsigned)(p[1]) << 8)
             | ((unsigned)(p[2]) << 16) | ((unsigned)(p[3]) << 24));
}
unsigned short rdU16(const unsigned char *p)
{
    return (unsigned short)(p[0] | (p[1] << 8));
}
void wrI32(unsigned char *p, int v)
{
    const unsigned u = (unsigned)v;
    p[0] = u & 0xFF; p[1] = (u >> 8) & 0xFF;
    p[2] = (u >> 16) & 0xFF; p[3] = (u >> 24) & 0xFF;
}
void wrU16(unsigned char *p, unsigned short v)
{
    p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF;
}

} // namespace

KCamera::KCamera(QObject *parent) : QObject(parent)
{
    m_pCameraInfo = new _CameraInfoStruct();
    m_pEepromInfo = new _CameraEepromInfo();
}

KCamera::~KCamera()
{
    delete m_pCameraInfo;
    delete m_pEepromInfo;
}

KCamera *GetCamera()
{
    static KCamera inst;   // реф.: function-local static + guard + __cxa_atexit
    return &inst;
}

// --- состояние --------------------------------------------------------------

bool KCamera::IsCameraReady() const { return m_nCameraStatus == 4; }
bool KCamera::CameraIsNotReady() const { return !IsCameraReady(); }
bool KCamera::IsNeedRollover() const { return m_bNeedRollover; }

void KCamera::SetCameraStatus(int s)
{
    if (s == m_nCameraStatus)
        return;                       // реф.: ранний выход на совпадении
    m_nCameraStatus = s;
    if (s == 0) {
        ResetCameraInfo();
        ResetCameraEepromData();
        m_nCameraInfoSaveRet = 1;     // ⚠️ именно 1, и ДО отправки сигнала
    }
    emit CameraStatusChanged(s);
}

// --- проверка модели --------------------------------------------------------

bool KCamera::IsCameraMatch(const QString &model, const QString &sn) const
{
    // 1) Привилегированный обход — ДО любой валидации.
    if (int(KAccount::GetInstance().CurrentRole()) > 2)
        return true;
    if (model.isEmpty()) {
        LogPrintfEx(true, "[APP][E]: ", "Camera check: the camera model is empty.\n");
        return false;
    }
    if (sn.isEmpty()) {
        LogPrintfEx(true, "[APP][E]: ", "Camera check: the camera SN is empty.\n");
        return false;
    }
    // ⚠️ Белый список — ТОЧНОЕ равенство, регистрозависимо; карта серий тут
    // ни при чём. Значение серийника не сравнивается никогда.
    if (KEncStyle().IsCameraValid(model))
        return true;
    LogPrintfEx(true, "[APP][E]: ",
                "Camera check: the camera model is not in the list.\n");
    return false;
}

const QMap<QString, QString> &KCamera::GetCameraSeriesMap() { return kCameraSeriesMap; }

// --- разбор CID -------------------------------------------------------------

void KCamera::ExtractCameraInfo(const unsigned char *pData)
{
    _CameraInfoStruct *ci = m_pCameraInfo;
    ci->sModel    = cidStr(pData + 0x00, 16).trimmed();   // ЕДИНСТВЕННЫЙ trimmed
    ci->sSerialNo = cidStr(pData + 0x10, 12);
    // Байты 0x1c..0x21 ПРОПУСКАЮТСЯ.
    ci->s10 = cidStr(pData + 0x22, 8);
    ci->s18 = cidStr(pData + 0x2a, 16);
    ci->s20 = cidStr(pData + 0x3a, 60);

    const bool match = IsCameraMatch(ci->sModel, ci->sSerialNo);
    SetCameraStatus(match ? 4 : 5);
    // ⚠️ Точное регистрозависимое сравнение — отсюда и берётся «rollover».
    m_bNeedRollover = (ci->sModel == QLatin1String("10-100-201"));
}

// --- разбор страницы EEPROM -------------------------------------------------

void KCamera::ExtractFixDataPage(const unsigned char *pData)
{
    const unsigned char f = pData[0];
    // Годно, если флаги не 0x00 и не 0xFF (в реф. закодировано как
    // (uint8)(f - 1) <= 0xFD).
    if (f == 0x00 || f == 0xFF) {
        LogPrintfEx(true, "[APP][E]: ", "Camera: no video param saved.\n");
        m_pEepromInfo->flags = 0;
        return;
    }
    m_pEepromInfo->flags = f;
    if (f & 0x01) {
        m_pEepromInfo->centorX = rdI32(pData + 1);
        m_pEepromInfo->centorY = rdI32(pData + 5);
        LogPrintf("[APP][I]: ", "Camera: Read centor point: (%d,%d).\n",
                  m_pEepromInfo->centorX, m_pEepromInfo->centorY);
        emit EepromParamUpdated(1);
    }
    if (f & 0x02) {
        m_pEepromInfo->rGain = rdU16(pData + 0x09);
        // ⚠️ p[0x0b]/p[0x0c] ПРОПУСКАЮТСЯ — похоже на сдвиг на два байта,
        // но именно так в бинарнике.
        m_pEepromInfo->bGain = rdU16(pData + 0x0d);
        emit EepromParamUpdated(0);
    } else {
        LogPrintfEx(true, "[APP][E]: ", "Camera: no white balance param saved.\n");
    }
}

bool KCamera::IsEepromDataOK() const
{
    return m_pEepromInfo->flags != 0x00 && m_pEepromInfo->flags != 0xFF;
}

void KCamera::SetVideoCentorPoint(int x, int y)
{
    m_pEepromInfo->centorX = x;
    m_pEepromInfo->centorY = y;
    m_pEepromInfo->flags |= 0x01;
    SaveFixParam();
}

void KCamera::SetWhBPara(unsigned short r, unsigned short b)
{
    m_pEepromInfo->rGain = r;
    m_pEepromInfo->bGain = b;
    m_pEepromInfo->flags |= 0x02;
    SaveFixParam();
}

void KCamera::SaveFixParam()
{
    unsigned char *p = m_eepromSaveData;
    memset(p, 0, sizeof(m_eepromSaveData));
    p[0] = m_pEepromInfo->flags;
    wrI32(p + 1, m_pEepromInfo->centorX);
    wrI32(p + 5, m_pEepromInfo->centorY);
    wrU16(p + 0x09, m_pEepromInfo->rGain);
    // ⚠️ Зеркало квирка чтения: 0x0b и 0x0c остаются нулями.
    wrU16(p + 0x0d, m_pEepromInfo->bGain);
    emit SaveCameraEepromData(511);
}

void KCamera::ResetCameraInfo()
{
    *m_pCameraInfo = _CameraInfoStruct();
}

void KCamera::ResetCameraEepromData()
{
    // ⚠️ Дефолты НЕ нулевые.
    m_pEepromInfo->flags = 0;
    m_pEepromInfo->centorX = 16;
    m_pEepromInfo->centorY = 16;
    m_pEepromInfo->rGain = 18000;   // 0x4650
    m_pEepromInfo->bGain = 11800;   // 0x2E18
}

void KCamera::ClearCameraInfo()
{
    m_pCameraInfo->s20 = QStringLiteral("Cleared by R&D!");
}

// --- типы -------------------------------------------------------------------

int KCamera::GetCameraType() const { return 8; }     // жёсткая константа реф.
int KCamera::GetSensorType() const { return 2; }
int KCamera::GetFirmwareType() const { return 2; }

void KCamera::GetCentorPointStart(int &x, int &y) const
{
    // Реф. считает по firmwareType, но тот жёстко равен 2 ⇒ всегда (1, 7).
    const int ft = GetFirmwareType();
    x = (ft == 2) ? 1 : 0;
    y = (ft == 2) ? 7 : 0;
}
