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
