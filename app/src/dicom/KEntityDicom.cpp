#include "dicom/KEntityDicom.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QVariant>
#include <QStringList>
#include <QDebug>

namespace {
const char *kConn = "endo_dicom";
}

KEntityDicom &KEntityDicom::Instance()
{
    static KEntityDicom inst;
    return inst;
}

bool KEntityDicom::OpenDb(const QString &dbPath, const QString &worklistFieldMapXml)
{
    CloseDb();
    if (!worklistFieldMapXml.isEmpty())
        wlMap_.Load(worklistFieldMapXml);

    QSqlDatabase db = QSqlDatabase::contains(kConn)
        ? QSqlDatabase::database(kConn)
        : QSqlDatabase::addDatabase("QSQLITE", kConn);
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        qWarning() << "KEntityDicom::OpenDb:" << db.lastError().text();
        return false;
    }
    opened_ = true;
    return createTables();
}

void KEntityDicom::CloseDb()
{
    if (QSqlDatabase::contains(kConn)) {
        { QSqlDatabase::database(kConn).close(); }
        QSqlDatabase::removeDatabase(kConn);
    }
    opened_ = false;
}

bool KEntityDicom::createTables()
{
    QSqlQuery q(QSqlDatabase::database(kConn));

    // tb_DcmWorklist — колонки из WorklistFieldMap.xml (config-driven); ключ —
    // AccessionNumber. Если маппинг не загружен, создаём минимальный набор.
    QVector<QString> cols = wlMap_.ColumnNames();
    if (cols.isEmpty())
        cols = {"AccessionNumber", "PatientID", "PatientName", "PatientSex",
                "PatientBirthDate", "StudyInstanceUID", "Modality",
                "ScheduledProcedureStepStartDate"};
    QStringList defs;
    for (const QString &c : cols)
        defs << (c == wlKey_ ? QString("%1 TEXT PRIMARY KEY").arg(c)
                             : QString("%1 TEXT").arg(c));
    if (!cols.contains(wlKey_))
        defs.prepend(wlKey_ + " TEXT PRIMARY KEY");
    bool ok = q.exec("CREATE TABLE IF NOT EXISTS tb_DcmWorklist (" + defs.join(", ") + ")");

    // tb_DcmStore — очередь отправки Secondary Capture.
    ok = q.exec(
        "CREATE TABLE IF NOT EXISTS tb_DcmStore ("
        "SopInstanceUID TEXT PRIMARY KEY, ExamId TEXT, StudyInstanceUID TEXT, "
        "SeriesInstanceUID TEXT, FilePath TEXT, ServerName TEXT, "
        "SendStatus INTEGER, RetryCount INTEGER)") && ok;

    // tb_DcmStudy / tb_DcmSeries — иерархия DICOM (Study→Series→SOP).
    ok = q.exec(
        "CREATE TABLE IF NOT EXISTS tb_DcmStudy ("
        "StudyInstanceUID TEXT PRIMARY KEY, StudyID TEXT, StudyDate TEXT, "
        "StudyTime TEXT, StudyDescription TEXT, Modality TEXT, PatientID TEXT)") && ok;
    ok = q.exec(
        "CREATE TABLE IF NOT EXISTS tb_DcmSeries ("
        "SeriesInstanceUID TEXT PRIMARY KEY, StudyInstanceUID TEXT, SeriesNumber INTEGER, "
        "SeriesDate TEXT, SeriesDescription TEXT, Modality TEXT, "
        "NumberOfSeriesRelatedInstances INTEGER)") && ok;

    // tb_DcmMpps — Modality Performed Procedure Step; tb_DcmCommit — Storage Commitment.
    ok = q.exec(
        "CREATE TABLE IF NOT EXISTS tb_DcmMpps ("
        "MppsUID TEXT PRIMARY KEY, ExamId TEXT, PerformedProcedureStepID TEXT, "
        "PerformedProcedureStepStatus TEXT, StartDate TEXT, StartTime TEXT, "
        "EndDate TEXT, EndTime TEXT, Description TEXT)") && ok;
    ok = q.exec(
        "CREATE TABLE IF NOT EXISTS tb_DcmCommit ("
        "TransactionUID TEXT PRIMARY KEY, ExamId TEXT, SopInstanceUID TEXT, "
        "CommitStatus INTEGER)") && ok;

    if (!ok) qWarning() << "KEntityDicom::createTables:" << q.lastError().text();
    return ok;
}

// --- Worklist ---

bool KEntityDicom::CreateWorklistEntity(const QMap<QString, QString> &row)
{
    if (row.isEmpty()) return false;
    QStringList cols, ph;
    for (auto it = row.constBegin(); it != row.constEnd(); ++it) {
        cols << it.key();
        ph << "?";
    }
    QSqlQuery q(QSqlDatabase::database(kConn));
    // реф.: insert into %s (%s) values(%s)
    q.prepare(QString("INSERT OR REPLACE INTO tb_DcmWorklist (%1) VALUES (%2)")
                  .arg(cols.join(", "), ph.join(", ")));
    for (auto it = row.constBegin(); it != row.constEnd(); ++it)
        q.addBindValue(it.value());
    if (!q.exec()) {
        qWarning() << "CreateWorklistEntity:" << q.lastError().text();
        return false;
    }
    return true;
}

QMap<QString, QString> KEntityDicom::GetWorklistEntity(const QString &accessionNumber) const
{
    QMap<QString, QString> out;
    QSqlQuery q(QSqlDatabase::database(kConn));
    q.prepare(QString("SELECT * FROM tb_DcmWorklist WHERE %1 = ?").arg(wlKey_));
    q.addBindValue(accessionNumber);
    if (q.exec() && q.next()) {
        const QSqlRecord rec = q.record();
        for (int i = 0; i < rec.count(); ++i)
            out.insert(rec.fieldName(i), q.value(i).toString());
    }
    return out;
}

QList<QMap<QString, QString>> KEntityDicom::GetWorklistSummaryList() const
{
    QList<QMap<QString, QString>> list;
    QSqlQuery q(QSqlDatabase::database(kConn));
    if (!q.exec("SELECT * FROM tb_DcmWorklist"))
        return list;
    while (q.next()) {
        const QSqlRecord rec = q.record();
        QMap<QString, QString> row;
        for (int i = 0; i < rec.count(); ++i)
            row.insert(rec.fieldName(i), q.value(i).toString());
        list.append(row);
    }
    return list;
}

int KEntityDicom::GetWorklistNumber() const
{
    QSqlQuery q(QSqlDatabase::database(kConn));
    // реф.: select count(*) from tb_DcmWorklist;
    if (q.exec("SELECT count(*) FROM tb_DcmWorklist") && q.next())
        return q.value(0).toInt();
    return 0;
}

bool KEntityDicom::DeleteWorklistEntity(const QString &accessionNumber)
{
    QSqlQuery q(QSqlDatabase::database(kConn));
    q.prepare(QString("DELETE FROM tb_DcmWorklist WHERE %1 = ?").arg(wlKey_));
    q.addBindValue(accessionNumber);
    return q.exec();
}

bool KEntityDicom::ClearWorklist()
{
    // реф.: перед сохранением новой выборки worklist БД очищается.
    QSqlQuery q(QSqlDatabase::database(kConn));
    return q.exec("DELETE FROM tb_DcmWorklist");
}

// --- Store queue ---

bool KEntityDicom::CreateStoreEntity(const DcmStoreEntity &e)
{
    QSqlQuery q(QSqlDatabase::database(kConn));
    q.prepare("INSERT OR REPLACE INTO tb_DcmStore "
              "(SopInstanceUID, ExamId, StudyInstanceUID, SeriesInstanceUID, "
              "FilePath, ServerName, SendStatus, RetryCount) "
              "VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
    q.addBindValue(e.sopInstanceUID);
    q.addBindValue(e.examId);
    q.addBindValue(e.studyInstanceUID);
    q.addBindValue(e.seriesInstanceUID);
    q.addBindValue(e.filePath);
    q.addBindValue(e.serverName);
    q.addBindValue(e.sendStatus);
    q.addBindValue(e.retryCount);
    if (!q.exec()) {
        qWarning() << "CreateStoreEntity:" << q.lastError().text();
        return false;
    }
    return true;
}

QList<DcmStoreEntity> KEntityDicom::GetStoreListByExam(const QString &examId) const
{
    QList<DcmStoreEntity> list;
    QSqlQuery q(QSqlDatabase::database(kConn));
    q.prepare("SELECT SopInstanceUID, ExamId, StudyInstanceUID, SeriesInstanceUID, "
              "FilePath, ServerName, SendStatus, RetryCount FROM tb_DcmStore "
              "WHERE ExamId = ?");
    q.addBindValue(examId);
    if (!q.exec()) return list;
    while (q.next()) {
        DcmStoreEntity e;
        e.sopInstanceUID    = q.value(0).toString();
        e.examId            = q.value(1).toString();
        e.studyInstanceUID  = q.value(2).toString();
        e.seriesInstanceUID = q.value(3).toString();
        e.filePath          = q.value(4).toString();
        e.serverName        = q.value(5).toString();
        e.sendStatus        = q.value(6).toInt();
        e.retryCount        = q.value(7).toInt();
        list.append(e);
    }
    return list;
}

bool KEntityDicom::UpdateStoreStatus(const QString &sopInstanceUID, int status, int retryCount)
{
    QSqlQuery q(QSqlDatabase::database(kConn));
    q.prepare("UPDATE tb_DcmStore SET SendStatus = ?, RetryCount = ? "
              "WHERE SopInstanceUID = ?");
    q.addBindValue(status);
    q.addBindValue(retryCount);
    q.addBindValue(sopInstanceUID);
    return q.exec();
}

int KEntityDicom::GetStoreNumber() const
{
    QSqlQuery q(QSqlDatabase::database(kConn));
    if (q.exec("SELECT count(*) FROM tb_DcmStore") && q.next())
        return q.value(0).toInt();
    return 0;
}

// --- Study / Series ---

bool KEntityDicom::CreateStudyEntity(const DcmStudyEntity &e)
{
    QSqlQuery q(QSqlDatabase::database(kConn));
    q.prepare("INSERT OR REPLACE INTO tb_DcmStudy (StudyInstanceUID, StudyID, StudyDate, "
              "StudyTime, StudyDescription, Modality, PatientID) VALUES (?,?,?,?,?,?,?)");
    q.addBindValue(e.studyInstanceUID); q.addBindValue(e.studyID);
    q.addBindValue(e.studyDate);        q.addBindValue(e.studyTime);
    q.addBindValue(e.studyDescription); q.addBindValue(e.modality);
    q.addBindValue(e.patientId);
    if (!q.exec()) { qWarning() << "CreateStudyEntity:" << q.lastError().text(); return false; }
    return true;
}

bool KEntityDicom::GetStudyEntity(const QString &studyInstanceUID, DcmStudyEntity &out) const
{
    QSqlQuery q(QSqlDatabase::database(kConn));
    q.prepare("SELECT StudyInstanceUID, StudyID, StudyDate, StudyTime, StudyDescription, "
              "Modality, PatientID FROM tb_DcmStudy WHERE StudyInstanceUID=?");
    q.addBindValue(studyInstanceUID);
    if (!q.exec() || !q.next())
        return false;
    out.studyInstanceUID = q.value(0).toString(); out.studyID = q.value(1).toString();
    out.studyDate = q.value(2).toString();        out.studyTime = q.value(3).toString();
    out.studyDescription = q.value(4).toString(); out.modality = q.value(5).toString();
    out.patientId = q.value(6).toString();
    return true;
}

QList<DcmStudyEntity> KEntityDicom::GetStudiesByPatient(const QString &patientId) const
{
    QList<DcmStudyEntity> list;
    QSqlQuery q(QSqlDatabase::database(kConn));
    q.prepare("SELECT StudyInstanceUID, StudyID, StudyDate, StudyTime, StudyDescription, "
              "Modality, PatientID FROM tb_DcmStudy WHERE PatientID=? ORDER BY StudyDate DESC");
    q.addBindValue(patientId);
    if (!q.exec()) return list;
    while (q.next()) {
        DcmStudyEntity e;
        e.studyInstanceUID = q.value(0).toString(); e.studyID = q.value(1).toString();
        e.studyDate = q.value(2).toString();        e.studyTime = q.value(3).toString();
        e.studyDescription = q.value(4).toString(); e.modality = q.value(5).toString();
        e.patientId = q.value(6).toString();
        list.append(e);
    }
    return list;
}

bool KEntityDicom::CreateSeriesEntity(const DcmSeriesEntity &e)
{
    QSqlQuery q(QSqlDatabase::database(kConn));
    q.prepare("INSERT OR REPLACE INTO tb_DcmSeries (SeriesInstanceUID, StudyInstanceUID, "
              "SeriesNumber, SeriesDate, SeriesDescription, Modality, "
              "NumberOfSeriesRelatedInstances) VALUES (?,?,?,?,?,?,?)");
    q.addBindValue(e.seriesInstanceUID); q.addBindValue(e.studyInstanceUID);
    q.addBindValue(e.seriesNumber);      q.addBindValue(e.seriesDate);
    q.addBindValue(e.seriesDescription); q.addBindValue(e.modality);
    q.addBindValue(e.numberOfInstances);
    if (!q.exec()) { qWarning() << "CreateSeriesEntity:" << q.lastError().text(); return false; }
    return true;
}

QList<DcmSeriesEntity> KEntityDicom::GetSeriesByStudy(const QString &studyInstanceUID) const
{
    QList<DcmSeriesEntity> list;
    QSqlQuery q(QSqlDatabase::database(kConn));
    q.prepare("SELECT SeriesInstanceUID, StudyInstanceUID, SeriesNumber, SeriesDate, "
              "SeriesDescription, Modality, NumberOfSeriesRelatedInstances "
              "FROM tb_DcmSeries WHERE StudyInstanceUID=? ORDER BY SeriesNumber");
    q.addBindValue(studyInstanceUID);
    if (!q.exec()) return list;
    while (q.next()) {
        DcmSeriesEntity e;
        e.seriesInstanceUID = q.value(0).toString(); e.studyInstanceUID = q.value(1).toString();
        e.seriesNumber = q.value(2).toInt();         e.seriesDate = q.value(3).toString();
        e.seriesDescription = q.value(4).toString(); e.modality = q.value(5).toString();
        e.numberOfInstances = q.value(6).toInt();
        list.append(e);
    }
    return list;
}

int KEntityDicom::GetStudyNumber() const
{
    QSqlQuery q(QSqlDatabase::database(kConn));
    if (q.exec("SELECT count(*) FROM tb_DcmStudy") && q.next())
        return q.value(0).toInt();
    return 0;
}

// --- MPPS / Commit ---

bool KEntityDicom::CreateMppsEntity(const DcmMppsEntity &e)
{
    QSqlQuery q(QSqlDatabase::database(kConn));
    q.prepare("INSERT OR REPLACE INTO tb_DcmMpps (MppsUID, ExamId, PerformedProcedureStepID, "
              "PerformedProcedureStepStatus, StartDate, StartTime, EndDate, EndTime, "
              "Description) VALUES (?,?,?,?,?,?,?,?,?)");
    q.addBindValue(e.mppsUID);   q.addBindValue(e.examId);    q.addBindValue(e.stepID);
    q.addBindValue(e.status);    q.addBindValue(e.startDate); q.addBindValue(e.startTime);
    q.addBindValue(e.endDate);   q.addBindValue(e.endTime);   q.addBindValue(e.description);
    if (!q.exec()) { qWarning() << "CreateMppsEntity:" << q.lastError().text(); return false; }
    return true;
}

bool KEntityDicom::UpdateMppsStatus(const QString &mppsUID, const QString &status,
                                    const QString &endDate, const QString &endTime)
{
    QSqlQuery q(QSqlDatabase::database(kConn));
    q.prepare("UPDATE tb_DcmMpps SET PerformedProcedureStepStatus=?, EndDate=?, EndTime=? "
              "WHERE MppsUID=?");
    q.addBindValue(status); q.addBindValue(endDate); q.addBindValue(endTime);
    q.addBindValue(mppsUID);
    return q.exec();
}

bool KEntityDicom::GetMppsEntity(const QString &mppsUID, DcmMppsEntity &out) const
{
    QSqlQuery q(QSqlDatabase::database(kConn));
    q.prepare("SELECT MppsUID, ExamId, PerformedProcedureStepID, PerformedProcedureStepStatus, "
              "StartDate, StartTime, EndDate, EndTime, Description FROM tb_DcmMpps WHERE MppsUID=?");
    q.addBindValue(mppsUID);
    if (!q.exec() || !q.next())
        return false;
    out.mppsUID = q.value(0).toString();   out.examId = q.value(1).toString();
    out.stepID = q.value(2).toString();    out.status = q.value(3).toString();
    out.startDate = q.value(4).toString(); out.startTime = q.value(5).toString();
    out.endDate = q.value(6).toString();   out.endTime = q.value(7).toString();
    out.description = q.value(8).toString();
    return true;
}

bool KEntityDicom::CreateCommitEntity(const DcmCommitEntity &e)
{
    QSqlQuery q(QSqlDatabase::database(kConn));
    q.prepare("INSERT OR REPLACE INTO tb_DcmCommit (TransactionUID, ExamId, SopInstanceUID, "
              "CommitStatus) VALUES (?,?,?,?)");
    q.addBindValue(e.transactionUID); q.addBindValue(e.examId);
    q.addBindValue(e.sopInstanceUID); q.addBindValue(e.commitStatus);
    if (!q.exec()) { qWarning() << "CreateCommitEntity:" << q.lastError().text(); return false; }
    return true;
}

bool KEntityDicom::UpdateCommitStatus(const QString &transactionUID, int status)
{
    QSqlQuery q(QSqlDatabase::database(kConn));
    q.prepare("UPDATE tb_DcmCommit SET CommitStatus=? WHERE TransactionUID=?");
    q.addBindValue(status); q.addBindValue(transactionUID);
    return q.exec();
}

bool KEntityDicom::GetCommitEntity(const QString &transactionUID, DcmCommitEntity &out) const
{
    QSqlQuery q(QSqlDatabase::database(kConn));
    q.prepare("SELECT TransactionUID, ExamId, SopInstanceUID, CommitStatus "
              "FROM tb_DcmCommit WHERE TransactionUID=?");
    q.addBindValue(transactionUID);
    if (!q.exec() || !q.next())
        return false;
    out.transactionUID = q.value(0).toString(); out.examId = q.value(1).toString();
    out.sopInstanceUID = q.value(2).toString(); out.commitStatus = q.value(3).toInt();
    return true;
}
