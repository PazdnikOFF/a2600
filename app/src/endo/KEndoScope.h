#pragma once

#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>

// Эндоскоп: разбор EEPROM/CID, модель-серия, контроль доступа
// (реф. KEndoScope, X-2600).
//
// РЕВЕРС: обычный QObject (vtable 14 слотов, единственная база), sizeof 0x80.
// ⚠️ ВАЖНО: класс НЕ ДЕЛАЕТ ВВОДА-ВЫВОДА ВООБЩЕ — ни open/ioctl/mmap/read/write,
// ни I2C. Байты EEPROM приходят ПАРАМЕТРОМ (uchar*), а запись наружу — это
// Qt-СИГНАЛЫ. Поэтому класс проверяем off-device целиком.
// (Пометка «нужен прибор» в ROADMAP Фазы E была НЕВЕРНА — как и у K3ADimming.)
//
// Синглтон отдаёт СВОБОДНАЯ функция GetEndoScope(), а не GetInstance().

// Реф. _EndoInfoStruct, sizeof 0x40. Заполняется ExtractEndoinfoFromdata
// (кроме sModel/nRotateType/fZoomRatio — их ставит CheckEndoInfo).
struct _EndoInfoStruct {
    QString sModel;             // 0x00 ⚠️ ExtractEndoinfoFromdata его ОЧИЩАЕТ и
                                //      НЕ ЗАПОЛНЯЕТ — модель ставит CheckEndoInfo
    QString sEndoID;            // 0x08 CID 0x10..0x1b
    unsigned char b10 = 0;      // 0x10 CID 0x1c   — семантика НЕ УСТАНОВЛЕНА
    unsigned char b11 = 0;      // 0x11 CID 0x1d   — НЕ УСТАНОВЛЕНА
    unsigned char b12 = 0;      // 0x12 CID 0x1e   — НЕ УСТАНОВЛЕНА
    unsigned char b13 = 0;      // 0x13 CID 0x1f   — НЕ УСТАНОВЛЕНА (ResetEndoInfo
                                //      его НЕ чистит — квирк)
    QString sWarrantyDate;      // 0x18 CID 0x22..0x29
    QString sServerContract;    // 0x20 CID 0x2a..0x39
    QString sComment;           // 0x28 CID 0x3a..0x75
    unsigned short u30 = 0;     // 0x30 CID 0x20..0x21 LE — НЕ УСТАНОВЛЕНА
    int nGlassType = 3;         // 0x34 CID 0x76; ДЕФОЛТ 3 (а не 0)
    int nRotateType = 0;        // 0x38 из KEncStyle::getScopeRotateType
    float fZoomRatio = 1.0f;    // 0x3c из KEncStyle::GetScopeZoomRatio
};

// Реф. _EepromInfo, sizeof 0x38.
struct _EepromInfo {
    unsigned char  fixFlags = 0;        // 0x00 биты 2/5/7 НИКОГДА не проверяются
    unsigned int   extraFlags = 0;      // 0x04 биты 2,4..31 не проверяются
    int            centorX = 0;         // 0x08
    int            centorY = 0;         // 0x0c
    int            videoCapX = 0;       // 0x10
    int            videoCapY = 0;       // 0x14
    unsigned int   octangleCut = 0;     // 0x18 ⚠️ hi16 = Y, lo16 = X
    short          roundCut = 0;        // 0x1c
    unsigned short wbRed = 0;           // 0x1e
    unsigned short wbBlue = 0;          // 0x20
    unsigned short usedCount = 0;       // 0x22
    unsigned short remainUseTimes = 0;  // 0x24
    unsigned short matchProcMask = 0;   // 0x26
    QString        deadline;            // 0x28
    QStringList    matchProcList;       // 0x30
};

class KEndoScope : public QObject
{
    Q_OBJECT
public:
    explicit KEndoScope(QObject *parent = nullptr);
    ~KEndoScope() override;

    // --- состояние ----------------------------------------------------------
    bool IsEndoReady() const;          // m_endoScopeStatus == 4
    // Реф.: (v != 0 && v != 5).
    bool IsEndoOutOfControl() const;
    void SetEndoScopeStatus(int v);
    void SetEndoControlStatus(int v);

    // --- принадлежность модели (ВСЕ полярности выверены дизасмом) -----------
    // Реф.: QStringList::contains, ТОЧНОЕ равенство, РЕГИСТРОЗАВИСИМО;
    // результат возвращается НАПРЯМУЮ ⇒ вхождение в список означает TRUE.
    static bool IsSuperfineEndo(const QString &model);   // 6 записей
    static bool IsEndoHasChannel(const QString &model);  // 4 записи
    // Реф.: аргумента НЕТ — читает m_pEndoInfo->sModel.
    // model.contains("430") || model.contains("500") || model == "ED-5GT"
    // (первые два — ПОДСТРОКА, третий — полное равенство; всё регистрозависимо).
    bool IsVideoCalReveral() const;
    // ⚠️ ИМЯ ВВОДИТ В ЗАБЛУЖДЕНИЕ: это НЕ проверка суффикса, а
    // `model.indexOf("430") != -1` — регистрозависимый поиск ПОДСТРОКИ.
    static bool IsEndoModelHaveSuffix(const QString &model);

    // Реф.: НИКАКОЙ логики "*N" здесь НЕТ (она живёт в KEncStyle::IsScopeValid
    // и CheckEndoInfo). Роль > 2 — привилегированный обход всех проверок.
    bool ISEndoScopeMatch(const QString &model, const QString &id) const;

    // --- серии --------------------------------------------------------------
    // QMap<QString,QString>; промах → ПУСТАЯ строка (сентинела нет).
    static const QMap<QString, QString> &GetEndoSeriesMap();
    // Реф.: РОВНО ОДНА запись — "ED-5GT" → "NJE".
    static const QMap<QString, QString> &GetEndoSeriesMapWuHan();

    // --- разбор CID ---------------------------------------------------------
    // ⚠️ Второй параметр во ВСЕХ Extract*-методах МЁРТВ (не читается никогда) —
    // сохранён в сигнатурах ради верности.
    // ⚠️ .trimmed() применяется ТОЛЬКО к байтам 0x00..0x0f; остальные четыре
    // строки НЕ обрезаются.
    void ExtractEndoinfoFromdata(const unsigned char *pData);
    // Реф. makeCRC4Endo — ВОПРЕКИ ИМЕНИ это НЕ CRC, а 8-битный бегущий XOR
    // (seed 0, без полинома) по байтам 0x00..0x7e; сверяется с байтом 0x7f.
    static unsigned char makeCRC4Endo(const unsigned char *pData);

    // --- разбор EEPROM ------------------------------------------------------
    void ExtractFixParam(const unsigned char *pData, unsigned char nUnused);
    void ExtractExtraFixParam(const unsigned char *pData, unsigned char nUnused);
    // ⚠️ ОПАСНЫЙ КВИРК ОРИГИНАЛА: pData[1] (uchar) НЕ ОГРАНИЧИВАЕТСЯ, а
    // переданная длина игнорируется ⇒ при байте 0 == 0xAA читает до ~3 КБ ЗА
    // границей буфера. И matchProcList.clear() выполняется ДО валидации, так
    // что мусор затирает хороший белый список без отката.
    // У нас длина буфера передаётся ЯВНО и OOB не допускается (см. .cpp).
    void ExactMatchProcessorEepromData(const unsigned char *pData, unsigned char nUnused,
                                       int nBufLen = 64);
    void SaveFixParam();                                   // обратное кодирование
    void SaveMatchProcessor2Eeprom(const QStringList &lst); // страница 508
    void ResetEepromData();
    const unsigned char *GetEepromSaveData() const { return m_eepromSaveData; }
    unsigned char *EepromSaveData() { return m_eepromSaveData; }

    // ⚠️ КВИРК: remainUseTimes-- выполняется, но при usedCount == 0xFFFF метод
    // выходит ДО SaveFixParam() — декремент теряется. И 0xFFFE инкрементится
    // ровно в «недействительный» сентинел 0xFFFF.
    void AddEndoUsedCount();

    // --- геометрия ----------------------------------------------------------
    // По типу прошивки: 0→(8,8); 1→ rotateType==2 ? (168,23) : (167,24);
    // 3→(167,23); 5→(328,72); 8→(329,73); ВСЕ ОСТАЛЬНЫЕ (включая 2 и 4)→(0,0).
    void GetCentorPointStart(int &x, int &y) const;
    int  GetCentorPointXRange() const;   // знаковое /2
    int  GetCentorPointYRange() const;

    // --- делегаты в KEncStyle (переиспользуем существующее) -----------------
    // Все четыре передают m_pEndoInfo->sModel и НЕ проверяют указатель на null.
    int GetEndoType() const;
    int GetSensorType() const;
    int GetEndoFirmwareType() const;
    int GetEndoShapeType() const;
    // ⚠️ НЕ делегирует: это кэш-геттер поля (в отличие от четвёрки выше),
    // и он null-безопасен. Дефолт 0.
    int GetRotateType() const;
    // ⚠️ Дефолт 3 (а не 0), тоже кэш-геттер.
    int GetEndoGlassType() const;
    // ⚠️ КВИРК: null-проверка и запасное 1.0f МЕРТВЫ — сразу после них идёт
    // безусловное разыменование p->sModel. 1.36f при superfine И
    // KSystemSet::GetCornerCutSize(model) == 1 — ВТОРАЯ ПОЛОВИНА УСЛОВИЯ У НАС
    // ОПУЩЕНА (KSystemSet::GetCornerCutSize не реализован). Отступление
    // задокументировано в .cpp и в PROGRESS §10.
    float GetZoomRatio() const;

    _EndoInfoStruct *EndoInfo() { return m_pEndoInfo; }
    _EepromInfo     *EepromData() { return m_pEepromData; }

signals:
    // 12 сигналов реф. (слотов/свойств/invokable нет).
    void SaveEndoInfoSig();
    void SaveEndoEepromData(unsigned short page);
    void EndoScopeStatusChanged(int v);
    void ShowEndoScopeStatus(int v);
    void UpdateEndoControlInfo(int v);
    void ShowEndoInfoEepromRecRet(int a, int b);
    void EndoScopeInfoExtractStatus(bool ok);
    void ShowEndoInfoCIDSaveRet(int v);
    void ShowEndoInfoEepromSaveRet(int a, int b);
    void EepromParamUpdated(int what);
    void ShowEndoInfoCIDRecRet(int v);
    void UpdateScopeIconStatus();

private:
    int   m_endoScopeStatus = 0;              // 0x10
    int   m_endoControlStatus = 0;            // 0x14
    bool  m_bCIDRecStatus = false;            // 0x18
    bool  m_bEeprom506RecStatus = false;      // 0x19 ⚠️ сеттер есть, НО МЁРТВ
    bool  m_bEeprom511RecStatus = false;      // 0x1a
    _EndoInfoStruct *m_pEndoInfo = nullptr;   // 0x20 куча, new 0x40
    _EepromInfo     *m_pEepromData = nullptr; // 0x28 куча, new 0x38
    unsigned char    m_eepromSaveData[64] = {};  // 0x30 страница-черновик
};

// Реф. свободная функция-синглтон (а НЕ GetInstance).
KEndoScope *GetEndoScope();
