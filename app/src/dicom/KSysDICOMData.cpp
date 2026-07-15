#include "dicom/KSysDICOMData.h"
#include "sys/KSystem.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonValue>

namespace LINK_DICOM {
const QString PORT                   = QStringLiteral("Port");
const QString CONN_TIMEOUT           = QStringLiteral("ConnectTimeout");
const QString NAME                   = QStringLiteral("Name");
const QString AE                     = QStringLiteral("AE");
const QString IP                     = QStringLiteral("IP");
const QString IS_ENABLE              = QStringLiteral("IsEnable");
const QString ADD_TIME               = QStringLiteral("AddTime");
const QString MAX_DL_NUM             = QStringLiteral("MaxDownloadNum");
const QString REQ_PROC_DESC          = QStringLiteral("ReqProcDesc");
const QString SNAP_SYNC_UPLOAD_IMAGE = QStringLiteral("SnapSyncUploadImage");
const QString UPLOAD_PDF_REPORT      = QStringLiteral("UploadPDFReport");
const QString LOCAL                  = QStringLiteral("Local");
const QString MPPS                   = QStringLiteral("MPPS");
const QString WORKLIST               = QStringLiteral("WorkList");
const QString STORAGE                = QStringLiteral("Storage");
const QString CMT_STORAGE            = QStringLiteral("CommitStorage");
// Реф. инициализирует, но ни одна функция бинарника к нему не обращается —
// AddTime форматирует вызывающий код (UI). Держим для полноты namespace.
const QString DATE_FORMAT            = QStringLiteral("yyyy-MM-dd hh:mm:ss");
} // namespace LINK_DICOM

// ————— KDICOMServiceBaseConf —————

KDICOMServiceBaseConf::KDICOMServiceBaseConf(DICOMServiceType type) : type_(type)
{
    Clear();
}

void KDICOMServiceBaseConf::Clear()
{
    isEnable = false;
    port     = 0;
    name.clear();
    ip.clear();
    ae.clear();
    addTime.clear();
    // type_ не трогаем — 1:1 с реф.
}

void KDICOMServiceBaseConf::ReadDICOMBaseConf(const QJsonObject &o)
{
    if (o.contains(LINK_DICOM::IS_ENABLE)) isEnable = o.value(LINK_DICOM::IS_ENABLE).toBool(false);
    if (o.contains(LINK_DICOM::PORT))      port     = o.value(LINK_DICOM::PORT).toInt(0);
    if (o.contains(LINK_DICOM::NAME))      name     = o.value(LINK_DICOM::NAME).toString();
    if (o.contains(LINK_DICOM::IP))        ip       = o.value(LINK_DICOM::IP).toString();
    if (o.contains(LINK_DICOM::AE))        ae       = o.value(LINK_DICOM::AE).toString();
    if (o.contains(LINK_DICOM::ADD_TIME))  addTime  = o.value(LINK_DICOM::ADD_TIME).toString();
}

void KDICOMServiceBaseConf::GetDICOMBaseConfJsonObj(QJsonObject &o) const
{
    o.insert(LINK_DICOM::IS_ENABLE, isEnable);
    o.insert(LINK_DICOM::PORT, port);
    o.insert(LINK_DICOM::NAME, name);
    o.insert(LINK_DICOM::IP, ip);
    o.insert(LINK_DICOM::AE, ae);
    o.insert(LINK_DICOM::ADD_TIME, addTime);
}

bool KDICOMServiceBaseConf::operator==(const KDICOMServiceBaseConf &r) const
{
    return isEnable == r.isEnable && port == r.port && name == r.name
        && ip == r.ip && ae == r.ae && addTime == r.addTime;
}

// ————— KDICOMLocalConf —————

void KDICOMLocalConf::Clear()
{
    port                = 104;
    connectTimeout      = 60;
    name.clear();
    ae                  = LINK_DICOM::AE;   // дефолт реф. — литерал "AE"
    uploadPDFReport     = false;
    snapSyncUploadImage = false;
}

void KDICOMLocalConf::ReadDICOMLocalConf(const QJsonObject &o)
{
    if (o.contains(LINK_DICOM::PORT))         port           = o.value(LINK_DICOM::PORT).toInt(0);
    if (o.contains(LINK_DICOM::CONN_TIMEOUT)) connectTimeout = o.value(LINK_DICOM::CONN_TIMEOUT).toInt(0);
    if (o.contains(LINK_DICOM::NAME))         name           = o.value(LINK_DICOM::NAME).toString();
    if (o.contains(LINK_DICOM::AE))           ae             = o.value(LINK_DICOM::AE).toString();
    if (o.contains(LINK_DICOM::SNAP_SYNC_UPLOAD_IMAGE))
        snapSyncUploadImage = o.value(LINK_DICOM::SNAP_SYNC_UPLOAD_IMAGE).toBool(false);
    if (o.contains(LINK_DICOM::UPLOAD_PDF_REPORT))
        uploadPDFReport = o.value(LINK_DICOM::UPLOAD_PDF_REPORT).toBool(false);
}

void KDICOMLocalConf::GetDICOMLocalConfJsonObj(QJsonObject &o) const
{
    // Порядок insert — как в реф.
    o.insert(LINK_DICOM::PORT, port);
    o.insert(LINK_DICOM::CONN_TIMEOUT, connectTimeout);
    o.insert(LINK_DICOM::NAME, name);
    o.insert(LINK_DICOM::AE, ae);
    o.insert(LINK_DICOM::SNAP_SYNC_UPLOAD_IMAGE, snapSyncUploadImage);
    o.insert(LINK_DICOM::UPLOAD_PDF_REPORT, uploadPDFReport);
}

bool KDICOMLocalConf::operator==(const KDICOMLocalConf &r) const
{
    return port == r.port && connectTimeout == r.connectTimeout && name == r.name
        && ae == r.ae && uploadPDFReport == r.uploadPDFReport
        && snapSyncUploadImage == r.snapSyncUploadImage;
}

// ————— KDICOMCommitStorageConf —————

KDICOMCommitStorageConf::KDICOMCommitStorageConf()
    : KDICOMServiceBaseConf(DICOMServiceType::CommitStorage)
{
    Clear();
}

void KDICOMCommitStorageConf::Clear()
{
    KDICOMServiceBaseConf::Clear();
    isEnable = true;    // отличие от Base (реф. ctor/Clear)
}

// ————— KDICOMWorkListConf —————

void KDICOMWorkListConf::Clear()
{
    KDICOMServiceBaseConf::Clear();
    maxDownloadNum = 99;
    reqProcDesc    = 0;
}

void KDICOMWorkListConf::ReadDICOMWorkListConf(const QJsonObject &o)
{
    ReadDICOMBaseConf(o);
    if (o.contains(LINK_DICOM::MAX_DL_NUM))    maxDownloadNum = o.value(LINK_DICOM::MAX_DL_NUM).toInt(0);
    if (o.contains(LINK_DICOM::REQ_PROC_DESC)) reqProcDesc    = o.value(LINK_DICOM::REQ_PROC_DESC).toInt(0);
}

void KDICOMWorkListConf::GetDICOMWorkListConfJsonObj(QJsonObject &o) const
{
    GetDICOMBaseConfJsonObj(o);
    o.insert(LINK_DICOM::MAX_DL_NUM, maxDownloadNum);
    o.insert(LINK_DICOM::REQ_PROC_DESC, reqProcDesc);
}

bool KDICOMWorkListConf::operator==(const KDICOMWorkListConf &r) const
{
    return KDICOMServiceBaseConf::operator==(r)
        && maxDownloadNum == r.maxDownloadNum && reqProcDesc == r.reqProcDesc;
}

// ————— KDICOMStorageConf —————

void KDICOMStorageConf::Clear()
{
    KDICOMServiceBaseConf::Clear();
    commitStorage.Clear();
}

void KDICOMStorageConf::ReadDICOMStorageConf(const QJsonObject &o)
{
    ReadDICOMBaseConf(o);
    if (o.contains(LINK_DICOM::CMT_STORAGE))
        commitStorage.ReadDICOMCommitStorageConf(o.value(LINK_DICOM::CMT_STORAGE).toObject());
}

void KDICOMStorageConf::GetDICOMStorageConfJsonObj(QJsonObject &o) const
{
    GetDICOMBaseConfJsonObj(o);
    QJsonObject cmt;
    commitStorage.GetDICOMCommitStorageConfJsonObj(cmt);
    o.insert(LINK_DICOM::CMT_STORAGE, cmt);
}

bool KDICOMStorageConf::operator==(const KDICOMStorageConf &r) const
{
    return KDICOMServiceBaseConf::operator==(r) && commitStorage == r.commitStorage;
}

// ————— KDICOMConf —————

void KDICOMConf::Clear()
{
    local_.Clear();
    mpps_.clear();
    worklist_.clear();
    storage_.clear();
    cmtStorage_.clear();
}

void KDICOMConf::ReadDicomConf(const QJsonObject &o)
{
    // Порядок и гейт по contains — 1:1 с реф.
    if (o.contains(LINK_DICOM::LOCAL))
        local_.ReadDICOMLocalConf(o.value(LINK_DICOM::LOCAL).toObject());

    if (o.contains(LINK_DICOM::MPPS)) {
        mpps_.clear();
        for (const QJsonValue &v : o.value(LINK_DICOM::MPPS).toArray()) {
            KDICOMMPPSConf c;
            c.ReadDICOMMPPSConf(v.toObject());
            mpps_.append(c);
        }
    }
    if (o.contains(LINK_DICOM::WORKLIST)) {
        worklist_.clear();
        for (const QJsonValue &v : o.value(LINK_DICOM::WORKLIST).toArray()) {
            KDICOMWorkListConf c;
            c.ReadDICOMWorkListConf(v.toObject());
            worklist_.append(c);
        }
    }
    if (o.contains(LINK_DICOM::STORAGE)) {
        storage_.clear();
        for (const QJsonValue &v : o.value(LINK_DICOM::STORAGE).toArray()) {
            KDICOMStorageConf c;
            c.ReadDICOMStorageConf(v.toObject());
            storage_.append(c);
        }
    }
    if (o.contains(LINK_DICOM::CMT_STORAGE)) {
        cmtStorage_.clear();
        for (const QJsonValue &v : o.value(LINK_DICOM::CMT_STORAGE).toArray()) {
            KDICOMCommitStorageConf c;
            c.ReadDICOMCommitStorageConf(v.toObject());
            cmtStorage_.append(c);
        }
    }
}

void KDICOMConf::GetDICOMConfJsonObj(QJsonObject &o) const
{
    QJsonObject local;
    local_.GetDICOMLocalConfJsonObj(local);
    o.insert(LINK_DICOM::LOCAL, local);

    QJsonArray mpps;
    for (const KDICOMMPPSConf &c : mpps_) {
        QJsonObject e;
        c.GetDICOMMPPSConfJsonObj(e);
        mpps.append(e);
    }
    o.insert(LINK_DICOM::MPPS, mpps);

    QJsonArray wl;
    for (const KDICOMWorkListConf &c : worklist_) {
        QJsonObject e;
        c.GetDICOMWorkListConfJsonObj(e);
        wl.append(e);
    }
    o.insert(LINK_DICOM::WORKLIST, wl);

    QJsonArray st;
    for (const KDICOMStorageConf &c : storage_) {
        QJsonObject e;
        c.GetDICOMStorageConfJsonObj(e);
        st.append(e);
    }
    o.insert(LINK_DICOM::STORAGE, st);

    QJsonArray cmt;
    for (const KDICOMCommitStorageConf &c : cmtStorage_) {
        QJsonObject e;
        c.GetDICOMCommitStorageConfJsonObj(e);
        cmt.append(e);
    }
    o.insert(LINK_DICOM::CMT_STORAGE, cmt);
}

QList<KDICOMStorageConf> KDICOMConf::GetDICOMStorageConfList(bool onlyEnable) const
{
    if (!onlyEnable) return storage_;
    QList<KDICOMStorageConf> out;
    for (const KDICOMStorageConf &c : storage_)
        if (c.isEnable) out.append(c);
    return out;
}

QList<KDICOMWorkListConf> KDICOMConf::GetDICOMWorkListConfList(bool onlyEnable) const
{
    if (!onlyEnable) return worklist_;
    QList<KDICOMWorkListConf> out;
    for (const KDICOMWorkListConf &c : worklist_)
        if (c.isEnable) out.append(c);
    return out;
}

QList<KDICOMMPPSConf> KDICOMConf::GetDICOMMPPSConfList(bool onlyEnable) const
{
    if (!onlyEnable) return mpps_;
    QList<KDICOMMPPSConf> out;
    for (const KDICOMMPPSConf &c : mpps_)
        if (c.isEnable) out.append(c);
    return out;
}

QList<KDICOMCommitStorageConf> KDICOMConf::GetDICOMCMTStorageConfList() const
{
    return cmtStorage_;   // реф. — без фильтра
}

// ————— KSysDICOMData —————

KSysDICOMData::KSysDICOMData()
{
    Load();   // реф. ctor
}

QString KSysDICOMData::GetConfigFileName() const
{
    return QStringLiteral("link-dicom.json");   // реф. — константа без префикса
}

QString KSysDICOMData::ConfigFilePath() const
{
    return QDir(KSystem::UserSetPath()).absoluteFilePath(GetConfigFileName());
}

void KSysDICOMData::Load()
{
    KDICOMConf::Clear();

    const QString path = ConfigFilePath();
    qInfo() << "KSysDICOMData file:" << path;

    if (!QFile::exists(path)) {
        // Реф. создаёт каталог, но файл НЕ пишет (Save отсюда не зовётся)
        // и всё равно идёт на открытие.
        QDir dir(KSystem::UserSetPath());
        if (!dir.exists()) QDir().mkpath(KSystem::UserSetPath());
    }

    QFile f(path);
    if (!f.open(QIODevice::ReadWrite)) {   // реф. флаг 3 = ReadWrite
        qWarning() << "open dicom file error, path:" << path;
        return;
    }
    const QByteArray data = f.readAll();
    f.close();

    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "parse dicom file error reason:" << err.errorString();
        return;
    }
    m_jsonObj = doc.object();
    ReadDicomConfFromJsonObj();
}

void KSysDICOMData::ReadDicomConfFromJsonObj()
{
    KDICOMConf::ReadDicomConf(m_jsonObj);   // реф. — читает кэш
}

void KSysDICOMData::Save()
{
    // Реф. сериализует КЭШ m_jsonObj, а не GetDICOMConfJsonObj() — т.е. пишет то,
    // что было загружено. Поведение сохранено 1:1 (правки полей без Save(conf) не летят в файл).
    const QString path = ConfigFilePath();
    QDir dir(KSystem::UserSetPath());
    if (!dir.exists()) QDir().mkpath(KSystem::UserSetPath());

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning() << "open dicom file error, path:" << path;
        return;
    }
    f.write(QJsonDocument(m_jsonObj).toJson());
    f.close();
}

void KSysDICOMData::Save(const KDICOMConf &conf)
{
    qInfo() << "KSysDICOMData Save";

    const QString path = ConfigFilePath();
    QDir dir(KSystem::UserSetPath());
    if (!dir.exists()) QDir().mkpath(KSystem::UserSetPath());

    QJsonObject o;
    conf.GetDICOMConfJsonObj(o);

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning() << "open dicom file error, path:" << path;
        return;
    }
    f.write(QJsonDocument(o).toJson());
    f.close();

    KDICOMConf::operator=(conf);   // реф. — m_jsonObj при этом НЕ обновляется
}
