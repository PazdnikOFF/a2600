#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>

// Конфиг DICOM-сервисов (реф. service/systemsettings/data/KSysDICOMData.cpp, X-2600).
//
// Файл: <data>/setdata/userset/link-dicom.json (KSystem::UserSetPath() +
// GetConfigFileName()). Корневой объект: Local (объект) + MPPS/WorkList/Storage/
// CommitStorage (массивы). Ключи 1:1 с namespace LINK_DICOM в бинарнике.
//
// Питает struct DicomSetting (KDicomInterface) и очередь tb_DcmStore (KEntityDicom).

// Ключи JSON (реф. namespace LINK_DICOM).
namespace LINK_DICOM {
extern const QString PORT;                    // "Port"
extern const QString CONN_TIMEOUT;            // "ConnectTimeout"
extern const QString NAME;                    // "Name"
extern const QString AE;                      // "AE"
extern const QString IP;                      // "IP"
extern const QString IS_ENABLE;               // "IsEnable"
extern const QString ADD_TIME;                // "AddTime"
extern const QString MAX_DL_NUM;              // "MaxDownloadNum"
extern const QString REQ_PROC_DESC;           // "ReqProcDesc"
extern const QString SNAP_SYNC_UPLOAD_IMAGE;  // "SnapSyncUploadImage"
extern const QString UPLOAD_PDF_REPORT;       // "UploadPDFReport"
extern const QString LOCAL;                   // "Local"
extern const QString MPPS;                    // "MPPS"
extern const QString WORKLIST;                // "WorkList"
extern const QString STORAGE;                 // "Storage"
extern const QString CMT_STORAGE;             // "CommitStorage"
extern const QString DATE_FORMAT;             // "yyyy-MM-dd hh:mm:ss"
} // namespace LINK_DICOM

// Тип сервиса — в реф. безымянный int-дискриминатор (поле +0x28), в JSON НЕ пишется:
// тип задаётся тем, в каком массиве лежит запись. Имя enum из дизасма не восстановимо.
enum class DICOMServiceType {
    Unknown       = -1,
    Storage       = 0,
    CommitStorage = 1,
    WorkList      = 2,
    Mpps          = 3,
};

// Общая часть узла-сервиса (реф. KDICOMServiceBaseConf).
class KDICOMServiceBaseConf
{
public:
    explicit KDICOMServiceBaseConf(DICOMServiceType type = DICOMServiceType::Unknown);
    virtual ~KDICOMServiceBaseConf() = default;

    virtual void Clear();                                   // реф. Clear (тип НЕ сбрасывает)
    virtual void ReadDICOMBaseConf(const QJsonObject &o);
    virtual void GetDICOMBaseConfJsonObj(QJsonObject &o) const;

    bool operator==(const KDICOMServiceBaseConf &r) const;
    bool operator!=(const KDICOMServiceBaseConf &r) const { return !(*this == r); }

    DICOMServiceType ServiceType() const { return type_; }

    // Поля в порядке смещений реф. (+0x00 bool, +0x04 int, далее QString).
    bool    isEnable = false;   // IsEnable
    int     port     = 0;       // Port
    QString name;               // Name
    QString ip;                 // IP
    QString ae;                 // AE
    QString addTime;            // AddTime

protected:
    DICOMServiceType type_;     // +0x28, не сериализуется
};

// Локальный узел (реф. KDICOMLocalConf — НЕ наследник Base, свой набор полей).
class KDICOMLocalConf
{
public:
    KDICOMLocalConf() { Clear(); }

    void Clear();                                           // дефолты реф.: 104/60/AE
    void ReadDICOMLocalConf(const QJsonObject &o);
    void GetDICOMLocalConfJsonObj(QJsonObject &o) const;

    bool operator==(const KDICOMLocalConf &r) const;
    bool operator!=(const KDICOMLocalConf &r) const { return !(*this == r); }

    int     port           = 104;    // Port           (дефолт 104)
    int     connectTimeout = 60;     // ConnectTimeout (дефолт 60)
    QString name;                    // Name
    QString ae;                      // AE             (дефолт "AE")
    bool    uploadPDFReport      = false;  // UploadPDFReport
    bool    snapSyncUploadImage  = false;  // SnapSyncUploadImage
};

// Commit-узел (реф. KDICOMCommitStorageConf) — своих полей нет; отличие от Base
// только в дефолте IsEnable=true.
class KDICOMCommitStorageConf : public KDICOMServiceBaseConf
{
public:
    KDICOMCommitStorageConf();
    void Clear() override;                                  // как Base + IsEnable=true
    void ReadDICOMCommitStorageConf(const QJsonObject &o) { ReadDICOMBaseConf(o); }
    void GetDICOMCommitStorageConfJsonObj(QJsonObject &o) const { GetDICOMBaseConfJsonObj(o); }
};

// MPPS-узел (реф. KDICOMMPPSConf) — своих полей нет, Read/Get = тейлколл в Base.
class KDICOMMPPSConf : public KDICOMServiceBaseConf
{
public:
    KDICOMMPPSConf() : KDICOMServiceBaseConf(DICOMServiceType::Mpps) {}
    void ReadDICOMMPPSConf(const QJsonObject &o) { ReadDICOMBaseConf(o); }
    void GetDICOMMPPSConfJsonObj(QJsonObject &o) const { GetDICOMBaseConfJsonObj(o); }
};

// Worklist-узел (реф. KDICOMWorkListConf).
class KDICOMWorkListConf : public KDICOMServiceBaseConf
{
public:
    KDICOMWorkListConf() : KDICOMServiceBaseConf(DICOMServiceType::WorkList) { Clear(); }

    void Clear() override;
    void ReadDICOMWorkListConf(const QJsonObject &o);
    void GetDICOMWorkListConfJsonObj(QJsonObject &o) const;

    bool operator==(const KDICOMWorkListConf &r) const;

    int maxDownloadNum = 99;   // MaxDownloadNum (дефолт 99)
    int reqProcDesc    = 0;    // ReqProcDesc — в реф. int (toInt/QJsonValue(int)), не строка
};

// Storage-узел (реф. KDICOMStorageConf) — содержит ВЛОЖЕННЫЙ commit-узел
// (JSON-ключ "CommitStorage" внутри записи Storage).
class KDICOMStorageConf : public KDICOMServiceBaseConf
{
public:
    KDICOMStorageConf() : KDICOMServiceBaseConf(DICOMServiceType::Storage) { Clear(); }

    void Clear() override;
    void ReadDICOMStorageConf(const QJsonObject &o);
    void GetDICOMStorageConfJsonObj(QJsonObject &o) const;

    bool operator==(const KDICOMStorageConf &r) const;

    KDICOMCommitStorageConf commitStorage;   // +0x30, ключ "CommitStorage"
};

// Полный конфиг (реф. KDICOMConf).
class KDICOMConf
{
public:
    KDICOMConf() { Clear(); }
    virtual ~KDICOMConf() = default;

    void Clear();
    void ReadDicomConf(const QJsonObject &o);                    // реф. ReadDicomConf
    void GetDICOMConfJsonObj(QJsonObject &o) const;              // реф. GetDICOMConfJsonObj

    KDICOMLocalConf GetDICOMLocalConf() const { return local_; }
    void SetDICOMLocalConf(const KDICOMLocalConf &c) { local_ = c; }

    // onlyEnable=true → фильтр по IsEnable (реф. ldrb/cbz), иначе полная копия.
    QList<KDICOMStorageConf>  GetDICOMStorageConfList(bool onlyEnable = false) const;
    QList<KDICOMWorkListConf> GetDICOMWorkListConfList(bool onlyEnable = false) const;
    QList<KDICOMMPPSConf>     GetDICOMMPPSConfList(bool onlyEnable = false) const;
    QList<KDICOMCommitStorageConf> GetDICOMCMTStorageConfList() const;  // всегда полная

    void SetDICOMStorageConfList(const QList<KDICOMStorageConf> &l)  { storage_ = l; }
    void SetDICOMWorkListConfList(const QList<KDICOMWorkListConf> &l) { worklist_ = l; }
    void SetDICOMMPPSConfList(const QList<KDICOMMPPSConf> &l)         { mpps_ = l; }
    void SetDICOMCMTStorageConfList(const QList<KDICOMCommitStorageConf> &l) { cmtStorage_ = l; }

protected:
    KDICOMLocalConf                local_;       // +0x00
    QList<KDICOMMPPSConf>          mpps_;        // +0x20
    QList<KDICOMWorkListConf>      worklist_;    // +0x28
    QList<KDICOMStorageConf>       storage_;     // +0x30
    QList<KDICOMCommitStorageConf> cmtStorage_;  // +0x38
};

// Файловый слой (реф. KSysDICOMData : KDICOMConf).
class KSysDICOMData : public KDICOMConf
{
public:
    KSysDICOMData();                                  // реф. ctor вызывает Load()

    QString GetConfigFileName() const;                // "link-dicom.json" (без префикса)
    QString ConfigFilePath() const;                   // UserSetPath() + имя файла

    void Load();                                      // реф. Load
    void Save();                                      // реф. Save() — пишет КЭШ m_jsonObj
    void Save(const KDICOMConf &conf);                // реф. Save(conf) — сериализует conf

    void ReadDicomConfFromJsonObj();                  // реф. — читает кэш m_jsonObj
    KDICOMConf GetDICOMConf() const { return *this; }

private:
    QJsonObject m_jsonObj;                            // +0x40 (кэш прочитанного документа)
};
