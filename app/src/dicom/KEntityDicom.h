#pragma once

#include <QString>
#include <QList>
#include <QMap>

#include "dicom/KDicomFieldMap.h"

// БД-слой DICOM (реф. KEntityDicom + tb_Dcm* таблицы). Бэкенд — Qt5::Sql
// (на устройстве SQLCipher). Две основные таблицы:
//   • tb_DcmWorklist — кэш результатов Modality Worklist (колонки из WorklistFieldMap.xml);
//   • tb_DcmStore    — очередь отправки Secondary Capture (C-STORE).
// Реф. методы: CreateEntity, GetEntityDetail, GetEntityDetailList,
// GetEntitySummaryList, GetEntityNumber, UpdateEntity, DeleteSelf.
//
// Запись worklist — динамический набор колонок (QMap dbname→значение), ключ —
// AccessionNumber (реф. m_strAccessionNumber). Элемент очереди — по ExamId.

// Исследование DICOM (реф. KDcmStudyEntity, tb_DcmStudy). Study содержит Series.
struct DcmStudyEntity {
    QString studyInstanceUID;   // ключ (StudyInstanceUID)
    QString studyID;
    QString studyDate;
    QString studyTime;
    QString studyDescription;
    QString modality;           // ES (endoscopy)
    QString patientId;          // связь с пациентом
};

// Серия DICOM (реф. KDcmSeriesEntity, tb_DcmSeries). Series принадлежит Study.
struct DcmSeriesEntity {
    QString seriesInstanceUID;  // ключ (SeriesInstanceUID)
    QString studyInstanceUID;   // FK → Study
    int     seriesNumber = 0;
    QString seriesDate;
    QString seriesDescription;
    QString modality;
    int     numberOfInstances = 0;   // NumberOfSeriesRelatedInstances
};

// MPPS — Modality Performed Procedure Step (реф. tb_DcmMpps/tb_DcmPerformedProcedureStep).
struct DcmMppsEntity {
    QString mppsUID;            // ключ (SOP Instance UID шага)
    QString examId;
    QString stepID;            // PerformedProcedureStepID
    QString status;            // IN PROGRESS / COMPLETED / DISCONTINUED
    QString startDate, startTime;
    QString endDate, endTime;
    QString description;
};

// Storage Commitment (реф. KDcmCommitEntity, tb_DcmCommit).
struct DcmCommitEntity {
    QString transactionUID;    // ключ
    QString examId;
    QString sopInstanceUID;
    int     commitStatus = 0;  // 0=ожидание,1=подтверждён,2=отказ
};

// Элемент очереди отправки (реф. поля KEntityDicom для tb_DcmStore).
struct DcmStoreEntity {
    QString examId;            // m_strExamId — ключ
    QString studyInstanceUID;
    QString seriesInstanceUID;
    QString sopInstanceUID;
    QString filePath;          // путь .dcm
    QString serverName;        // целевой SCP
    int     sendStatus = 0;    // 0=ожидание,1=в процессе,2=успех,3=ошибка
    int     retryCount = 0;
};

class KEntityDicom
{
public:
    static KEntityDicom &Instance();

    // Открыть/создать БД; загрузить маппинг worklist (для колонок tb_DcmWorklist).
    bool OpenDb(const QString &dbPath, const QString &worklistFieldMapXml = QString());
    void CloseDb();

    const KDicomFieldMap &WorklistMap() const { return wlMap_; }

    // --- Worklist (tb_DcmWorklist) ---
    // Запись = отображение dbname→значение (реф. динамический датасет→БД).
    bool CreateWorklistEntity(const QMap<QString, QString> &row);
    QMap<QString, QString> GetWorklistEntity(const QString &accessionNumber) const;
    QList<QMap<QString, QString>> GetWorklistSummaryList() const;
    int  GetWorklistNumber() const;                       // select count(*)
    bool DeleteWorklistEntity(const QString &accessionNumber);
    bool ClearWorklist();                                 // при обновлении worklist

    // --- Исследования/серии (tb_DcmStudy / tb_DcmSeries) ---
    bool CreateStudyEntity(const DcmStudyEntity &e);
    bool GetStudyEntity(const QString &studyInstanceUID, DcmStudyEntity &out) const;
    QList<DcmStudyEntity> GetStudiesByPatient(const QString &patientId) const;
    bool CreateSeriesEntity(const DcmSeriesEntity &e);
    QList<DcmSeriesEntity> GetSeriesByStudy(const QString &studyInstanceUID) const;
    int  GetStudyNumber() const;

    // --- MPPS / Storage Commitment (tb_DcmMpps / tb_DcmCommit) ---
    bool CreateMppsEntity(const DcmMppsEntity &e);
    bool UpdateMppsStatus(const QString &mppsUID, const QString &status,
                          const QString &endDate = QString(), const QString &endTime = QString());
    bool GetMppsEntity(const QString &mppsUID, DcmMppsEntity &out) const;
    bool CreateCommitEntity(const DcmCommitEntity &e);
    bool UpdateCommitStatus(const QString &transactionUID, int status);
    bool GetCommitEntity(const QString &transactionUID, DcmCommitEntity &out) const;

    // --- Очередь отправки (tb_DcmStore) ---
    bool CreateStoreEntity(const DcmStoreEntity &e);
    QList<DcmStoreEntity> GetStoreListByExam(const QString &examId) const;
    bool UpdateStoreStatus(const QString &sopInstanceUID, int status, int retryCount);
    int  GetStoreNumber() const;

private:
    KEntityDicom() = default;
    bool createTables();

    KDicomFieldMap wlMap_;
    QString        wlKey_ = "AccessionNumber";  // первичный ключ worklist
    bool           opened_ = false;
};
