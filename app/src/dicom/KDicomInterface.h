#pragma once

#include <QString>

#include <string>
#include <vector>
#include <QStringList>
#include <QList>

// Сетевой DICOM-интерфейс (реф. KDicomInterface + KStoreScu/KWorklistScu/
// KCommitScu/KMppsScu/KEchoScp). Реализация опирается на DCMTK (недоступна на
// десктопе → компилируется под флагом HAVE_DCMTK, как GStreamer-тракт).
//
// Ассоциации/датасеты строятся по XML-форматам из presetdata/userpreset/dicom/
// (WorklistDatasetFormat.xml, Mpps*DatasetFormat.xml) и маппятся в БД по FieldMap.

// Параметры узла/подключения (реф. struct DicomSetting).
struct DicomSetting {
    QString serverName;        // отображаемое имя узла
    QString localAETitle;      // LocalAETitle (наш AE)
    QString calledAETitle;     // AE удалённого SCP
    QString remoteIP;          // адрес SCP
    int     storePort   = 104; // порт C-STORE SCP
    int     worklistPort = 104;// порт worklist SCP
    int     mppsPort    = 104;
    int     commitPort  = 104;
    int     timeout     = 30;  // Timeout, сек
    int     maxPDU      = 16384;
    bool    enable      = false;
};

// Результат операции (реф. коды возврата DCMTK-обёрток).
enum class DicomResult { Ok = 0, NetworkError, AssociationRejected, Timeout, Failed };

class KDicomInterface
{
public:
    static KDicomInterface &GetInstance();

    void InitDicomSetting(const DicomSetting &s);   // реф. InitDicomSetting
    DicomSetting GetDicomSetting() const { return setting_; }
    void ResetDicomSetting();                        // реф. ResetDicomSetting

    // C-ECHO (проверка связи) — реф. DicomEcho/Ping/CheckNetwork.
    DicomResult DicomEcho(const DicomSetting &s);

    // C-STORE: отправить одну Secondary Capture (.dcm) — реф. DicomStore/DicomStorePicture.
    DicomResult DicomStore(const DicomSetting &s, const QString &dcmFilePath);
    // Отправить все снимки осмотра из очереди tb_DcmStore — реф. DicomStoreOneExam.
    DicomResult DicomStoreOneExam(const DicomSetting &s, const QString &examId);

    // Modality Worklist C-FIND: скачать список задач в tb_DcmWorklist —
    // реф. DownloadWorklist (использует Worklist*Format.xml + FieldMap).
    DicomResult DownloadWorklist(const DicomSetting &s, const QString &dcmDir);

    // MPPS N-CREATE/N-SET — реф. DicomMPPS. Storage Commitment — реф. DicomCommit.
    DicomResult DicomMPPS(const DicomSetting &s, const QString &examId);
    DicomResult DicomCommit(const DicomSetting &s, const QString &examId);

    // --- серии осмотра (реф. вызовы из KExamBussinessHandler) ---------------
    // Сеть/DCMTK — DEVICE-часть; off-device ведём журнал вызовов, чтобы
    // self-test проверял ПОСЛЕДОВАТЕЛЬНОСТЬ ровно как в оригинале.
    void ActivateSeries(const std::string &worklistUID, const std::string &examId);
    void EndSeries(const std::string &examId);
    void RebindWorklist(const std::string &examId, const std::string &worklistUID);
    void DicomStore(const std::string &examId);

    struct SeriesCall { std::string op, a, b; };
    std::vector<SeriesCall> TakeSeriesCalls() const;
    void ClearSeriesCalls();

private:
    KDicomInterface() = default;
    DicomSetting setting_;
};
