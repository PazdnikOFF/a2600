#pragma once

#include <QMap>
#include <QObject>
#include <QString>

// Камера: разбор CID/EEPROM, проверка модели (реф. KCamera, X-2600).
//
// РЕВЕРС: обычный QObject (vtable 14 слотов, единственная база), sizeof 0x78.
// ⚠️ Работы с железом НЕТ ВООБЩЕ — весь внешний вызов это LogPrintf(Ex),
// KAccount::CurrentRole, KEncStyle::IsCameraValid, usleep. Байты приходят
// параметром, наружу идут 7 Qt-сигналов ⇒ класс проверяем off-device целиком.
// (Пометка «нужен прибор» Фазы E была неверна — см. аудит в PROGRESS §10.)
//
// Синглтон отдаёт СВОБОДНАЯ функция GetCamera() (как и у KEndoScope).
//
// ЧЕМ ОТЛИЧАЕТСЯ ОТ KEndoScope (не копировать вслепую):
//   • карта серий ОДНА (у эндоскопа их две — основная и «ухань»);
//   • Get{Camera,Sensor,Firmware}Type возвращают ЖЁСТКИЕ КОНСТАНТЫ 8/2/2,
//     а НЕ делегируют в KEncStyle, как четвёрка Get*Type у KEndoScope;
//   • ExtractFixDataPage БЕЗ параметра длины;
//   • структура инфо — 5 обычных QString, без числовых полей и без QStringList;
//   • контрольной суммы в классе нет вообще (см. makeCRC4Endo у KEndoScope).

// Реф. _CameraInfoStruct, sizeof 0x28 — пять QString.
struct _CameraInfoStruct {
    QString sModel;      // 0x00 CID 0x00..0x0f, ЕДИНСТВЕННОЕ поле с .trimmed()
    QString sSerialNo;   // 0x08 CID 0x10..0x1b
    QString s10;         // 0x10 CID 0x22..0x29 — назначение НЕ УСТАНОВЛЕНО
    QString s18;         // 0x18 CID 0x2a..0x39 — НЕ УСТАНОВЛЕНО
    QString s20;         // 0x20 CID 0x3a..0x75 — НЕ УСТАНОВЛЕНО
};

// Реф. _CameraEepromInfo, sizeof 0x10. Байты 0x01..0x03 — выравнивание,
// НИКОГДА не пишутся.
struct _CameraEepromInfo {
    unsigned char  flags = 0;     // 0x00 бит0 = центр, бит1 = баланс белого
    int            centorX = 0;   // 0x04
    int            centorY = 0;   // 0x08
    unsigned short rGain = 0;     // 0x0c
    unsigned short bGain = 0;     // 0x0e
};

class KCamera : public QObject
{
    Q_OBJECT
public:
    explicit KCamera(QObject *parent = nullptr);
    ~KCamera() override;

    // --- состояние ----------------------------------------------------------
    bool IsCameraReady() const;      // m_nCameraStatus == 4, возвращается НАПРЯМУЮ
    bool CameraIsNotReady() const;   // строго !IsCameraReady() — ловушки нет
    // ⚠️ ИМЯ ВВОДИТ В ЗАБЛУЖДЕНИЕ (та же ловушка, что и IsEndoModelHaveSuffix):
    // это НЕ счётчик перевода, а признак «модель РОВНО "10-100-201"» (серия 4I6),
    // выставляемый в ExtractCameraInfo точным регистрозависимым сравнением.
    bool IsNeedRollover() const;
    // ⚠️ Ранний выход при s == текущего; при s == 0 сбрасывает обе структуры и
    // ставит m_nCameraInfoSaveRet = 1 (НЕ 0) ДО отправки сигнала.
    void SetCameraStatus(int s);
    int  GetCameraStatus() const { return m_nCameraStatus; }
    int  GetCameraInfoSaveRet() const { return m_nCameraInfoSaveRet; }

    // --- проверка модели ----------------------------------------------------
    // Реф.: `this` НЕ используется (фактически статический). Порядок проверок:
    // 1) роль > 2 → true БЕЗ какой-либо валидации (привилегированный обход);
    // 2) пустая модель → false; 3) пустой серийник → false;
    // 4) KEncStyle::IsCameraValid(model) — ТОЧНОЕ равенство по белому списку,
    //    РЕГИСТРОЗАВИСИМО (не подстрока и НЕ поиск по карте серий).
    // Значение серийника не сравнивается НИКОГДА — только непустота.
    bool IsCameraMatch(const QString &model, const QString &sn) const;

    // --- карта серий --------------------------------------------------------
    // Реф.: отдаётся ССЫЛКА на поле, без копии. Промах у QMap::value → пустая строка.
    static const QMap<QString, QString> &GetCameraSeriesMap();

    // --- разбор CID (128 байт полезной нагрузки MCU) ------------------------
    // ⚠️ Строки — NUL-ТЕРМИНИРОВАННЫЕ (strlen), а не фиксированной ширины.
    // ⚠️ .trimmed() применяется ТОЛЬКО к модели.
    // Байты 0x1c..0x21 ПРОПУСКАЮТСЯ; максимум читается p[0x75].
    // Контрольной суммы здесь нет — её проверяет вызывающий (HmiMcu) тем же
    // makeCRC4Endo, который на деле является XOR (см. KEndoScope).
    void ExtractCameraInfo(const unsigned char *pData);

    // --- разбор страницы EEPROM ---------------------------------------------
    // Годной считается страница с флагами НЕ 0x00 и НЕ 0xFF.
    // бит0 → центр (p[1..8], LE int32); бит1 → баланс белого
    // (rGain p[0x09..0x0a], bGain p[0x0d..0x0e] — ⚠️ p[0x0b]/p[0x0c]
    // ПРОПУСКАЮТСЯ, похоже на сдвиг на два байта, но так в бинарнике).
    void ExtractFixDataPage(const unsigned char *pData);
    bool IsEepromDataOK() const;   // flags не 0x00 и не 0xFF

    void SetVideoCentorPoint(int x, int y);   // ставит бит0 + SaveFixParam
    void SetWhBPara(unsigned short r, unsigned short b);  // бит1 + SaveFixParam
    void SaveFixParam();                       // → сигнал SaveCameraEepromData(511)
    void ResetCameraInfo();
    // ⚠️ Дефолты НЕ нулевые: центр (16,16), rGain 18000, bGain 11800.
    void ResetCameraEepromData();
    void ClearCameraInfo();      // пишет литерал "Cleared by R&D!" в s20

    const unsigned char *GetEepromSaveData() const { return m_eepromSaveData; }

    // --- типы: в реф. ЖЁСТКИЕ КОНСТАНТЫ, а не запрос в KEncStyle ------------
    int GetCameraType() const;     // всегда 8
    int GetSensorType() const;     // всегда 2
    int GetFirmwareType() const;   // всегда 2
    // ⚠️ Считает x = (ft == 2), y = (ft == 2) ? 7 : 0, но ft жёстко 2 ⇒
    // ВСЕГДА (1, 7). Диапазоны при этом всегда 0.
    void GetCentorPointStart(int &x, int &y) const;
    int  GetCentorPointXRange() const { return 0; }
    int  GetCentorPointYRange() const { return 0; }

    _CameraInfoStruct *GetCameraInfo() { return m_pCameraInfo; }
    _CameraEepromInfo *EepromInfo() { return m_pEepromInfo; }

signals:
    void SaveCameraInfoSig();
    void SaveCameraEepromData(unsigned short page);
    void CameraStatusChanged(int v);
    void ShowCameraStatus(int v);
    void ShowCameraInfoSaveRet(int v);
    void EepromParamUpdated(int what);
    void ShowCameraInfoCIDRecRet(int v);

private:
    int   m_nCameraStatus = 0;             // 0x10
    int   m_nCameraInfoSaveRet = 1;        // 0x14 ⚠️ ctor даёт 1, а не 0
    bool  m_bCameraInfoCIDRecStatus = false; // 0x18
    bool  m_bNeedRollover = false;         // 0x19 (два ОТДЕЛЬНЫХ bool, не u16)
    _CameraInfoStruct *m_pCameraInfo = nullptr;  // 0x20 куча, new 0x28
    _CameraEepromInfo *m_pEepromInfo = nullptr;  // 0x28 куча, new 0x10
    unsigned char m_eepromSaveData[64] = {};     // 0x30 черновая страница
};

// Реф. свободная функция-синглтон.
KCamera *GetCamera();
