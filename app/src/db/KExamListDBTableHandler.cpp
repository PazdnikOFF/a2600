#include "db/KExamListDBTableHandler.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QVariant>
#include <QDebug>

namespace exambiz {
const char *const kInvalidString = "INVALID_STRING";
}

void KExamEntry::ResetToInvalid()
{
    const std::string inv = exambiz::kInvalidString;
    PatientListTableKey = -1;
    PatientName = inv;      PatientSex = inv;
    PatientAge = -1;
    PatientBirthday = inv;  PatientID = inv;        ExamId = inv;
    ExamDate = inv;         DrExamName = inv;       DrExamId = inv;
    DrExamRole = inv;
    ExamType = -1;
    Device = inv;           DeviceSN = inv;         DrReportName = inv;
    DrReportId = inv;       DrReportRole = inv;     ApplicantDate = inv;
    Applicants = inv;       PlanDate = inv;         SickBedId = inv;
    TelephoneNumber = inv;  UserItem1 = inv;        UserItem2 = inv;
    ReportStatus = inv;
    ReportPrintTimes = -1;  UploadTimes = -1;
    RecordPath = inv;       RegisterNumber = inv;   Reserved1 = inv;
    Reserved2 = inv;        WorklistUID = inv;      ExamTime = inv;
    RecordImgNum = -1;      RecordVideoNum = -1;
}

namespace {

QString g_conn = QStringLiteral("endo_main");

// Порядок — как в реф. KExamEntry (по возрастанию смещения).
struct ColDef { const char *name; const char *type; };
const ColDef kColumns[] = {
    {"PatientListTableKey", "INTEGER"}, {"PatientName", "TEXT"},
    {"PatientSex", "TEXT"},             {"PatientAge", "INTEGER"},
    {"PatientBirthday", "TEXT"},        {"PatientID", "TEXT"},
    {"ExamId", "TEXT"},                 {"ExamDate", "TEXT"},
    {"DrExamName", "TEXT"},             {"DrExamId", "TEXT"},
    {"DrExamRole", "TEXT"},             {"ExamType", "INTEGER"},
    {"Device", "TEXT"},                 {"DeviceSN", "TEXT"},
    {"DrReportName", "TEXT"},           {"DrReportId", "TEXT"},
    {"DrReportRole", "TEXT"},           {"ApplicantDate", "TEXT"},
    {"Applicants", "TEXT"},             {"PlanDate", "TEXT"},
    {"SickBedId", "TEXT"},              {"TelephoneNumber", "TEXT"},
    {"UserItem1", "TEXT"},              {"UserItem2", "TEXT"},
    {"ReportStatus", "TEXT"},           {"ReportPrintTimes", "INTEGER"},
    {"UploadTimes", "INTEGER"},         {"RecordPath", "TEXT"},
    {"RegisterNumber", "TEXT"},         {"Reserved1", "TEXT"},
    {"Reserved2", "TEXT"},              {"WorklistUID", "TEXT"},
    {"ExamTime", "TEXT"},               {"RecordImgNum", "INTEGER"},
    {"RecordVideoNum", "INTEGER"},
    // Колонки «тонкого» KEntityExam, которых нет в реф. KExamEntry — держим,
    // чтобы обе реализации работали на одной таблице (см. заголовок).
    {"AccessionNumber", "TEXT"},        {"ExamStatus", "INTEGER"},
    {"ExamDir", "TEXT"},
};
const int kColumnCount = int(sizeof(kColumns) / sizeof(kColumns[0]));
// Колонки реф. KExamEntry — первые 35 из kColumns.
const int kRefColumnCount = 35;

QString refColumnList()
{
    QString s;
    for (int i = 0; i < kRefColumnCount; ++i) {
        if (i) s += ", ";
        s += QLatin1String(kColumns[i].name);
    }
    return s;
}

void bindEntry(QSqlQuery &q, const KExamEntry &e)
{
    const auto S = [](const std::string &v) { return QString::fromStdString(v); };
    q.addBindValue(e.PatientListTableKey); q.addBindValue(S(e.PatientName));
    q.addBindValue(S(e.PatientSex));       q.addBindValue(e.PatientAge);
    q.addBindValue(S(e.PatientBirthday));  q.addBindValue(S(e.PatientID));
    q.addBindValue(S(e.ExamId));           q.addBindValue(S(e.ExamDate));
    q.addBindValue(S(e.DrExamName));       q.addBindValue(S(e.DrExamId));
    q.addBindValue(S(e.DrExamRole));       q.addBindValue(e.ExamType);
    q.addBindValue(S(e.Device));           q.addBindValue(S(e.DeviceSN));
    q.addBindValue(S(e.DrReportName));     q.addBindValue(S(e.DrReportId));
    q.addBindValue(S(e.DrReportRole));     q.addBindValue(S(e.ApplicantDate));
    q.addBindValue(S(e.Applicants));       q.addBindValue(S(e.PlanDate));
    q.addBindValue(S(e.SickBedId));        q.addBindValue(S(e.TelephoneNumber));
    q.addBindValue(S(e.UserItem1));        q.addBindValue(S(e.UserItem2));
    q.addBindValue(S(e.ReportStatus));     q.addBindValue(e.ReportPrintTimes);
    q.addBindValue(e.UploadTimes);         q.addBindValue(S(e.RecordPath));
    q.addBindValue(S(e.RegisterNumber));   q.addBindValue(S(e.Reserved1));
    q.addBindValue(S(e.Reserved2));        q.addBindValue(S(e.WorklistUID));
    q.addBindValue(S(e.ExamTime));         q.addBindValue(e.RecordImgNum);
    q.addBindValue(e.RecordVideoNum);
}

KExamEntry fromQuery(const QSqlQuery &q)
{
    KExamEntry e;
    const auto S = [&q](int i) { return q.value(i).toString().toStdString(); };
    e.PatientListTableKey = q.value(0).toInt();  e.PatientName = S(1);
    e.PatientSex = S(2);                         e.PatientAge = q.value(3).toInt();
    e.PatientBirthday = S(4);                    e.PatientID = S(5);
    e.ExamId = S(6);                             e.ExamDate = S(7);
    e.DrExamName = S(8);                         e.DrExamId = S(9);
    e.DrExamRole = S(10);                        e.ExamType = q.value(11).toInt();
    e.Device = S(12);                            e.DeviceSN = S(13);
    e.DrReportName = S(14);                      e.DrReportId = S(15);
    e.DrReportRole = S(16);                      e.ApplicantDate = S(17);
    e.Applicants = S(18);                        e.PlanDate = S(19);
    e.SickBedId = S(20);                         e.TelephoneNumber = S(21);
    e.UserItem1 = S(22);                         e.UserItem2 = S(23);
    e.ReportStatus = S(24);                      e.ReportPrintTimes = q.value(25).toInt();
    e.UploadTimes = q.value(26).toInt();         e.RecordPath = S(27);
    e.RegisterNumber = S(28);                    e.Reserved1 = S(29);
    e.Reserved2 = S(30);                         e.WorklistUID = S(31);
    e.ExamTime = S(32);                          e.RecordImgNum = q.value(33).toInt();
    e.RecordVideoNum = q.value(34).toInt();
    return e;
}

} // namespace

void KExamListDBTableHandler::SetConnectionName(const QString &connectionName)
{
    g_conn = connectionName;
}

QString KExamListDBTableHandler::ConnectionName() { return g_conn; }

bool KExamListDBTableHandler::CreateTable(const QString &connectionName)
{
    g_conn = connectionName;
    QSqlDatabase db = QSqlDatabase::database(connectionName);
    QSqlQuery q(db);

    QString ddl = QStringLiteral("CREATE TABLE IF NOT EXISTS tb_ExamList (");
    for (int i = 0; i < kColumnCount; ++i) {
        if (i) ddl += ", ";
        ddl += QLatin1String(kColumns[i].name);
        ddl += ' ';
        ddl += QLatin1String(kColumns[i].type);
    }
    ddl += ')';
    if (!q.exec(ddl)) {
        qWarning() << "KExamListDBTableHandler::CreateTable:" << q.lastError().text();
        return false;
    }

    // Таблица могла быть создана «тонким» KEntityExam — добрать недостающие колонки.
    QStringList have;
    if (q.exec(QStringLiteral("PRAGMA table_info(tb_ExamList)")))
        while (q.next())
            have << q.value(1).toString().toLower();
    for (int i = 0; i < kColumnCount; ++i) {
        if (have.contains(QString(QLatin1String(kColumns[i].name)).toLower()))
            continue;
        QSqlQuery alter(db);
        if (!alter.exec(QStringLiteral("ALTER TABLE tb_ExamList ADD COLUMN %1 %2")
                            .arg(QLatin1String(kColumns[i].name),
                                 QLatin1String(kColumns[i].type))))
            qWarning() << "KExamListDBTableHandler::CreateTable ADD COLUMN"
                       << kColumns[i].name << alter.lastError().text();
    }
    return true;
}

int KExamListDBTableHandler::AddExamEntity(const KExamEntry &e)
{
    QSqlQuery q(QSqlDatabase::database(g_conn));
    QString marks;
    for (int i = 0; i < kRefColumnCount; ++i) marks += (i ? ", ?" : "?");
    q.prepare("INSERT INTO tb_ExamList (" + refColumnList() + ") VALUES (" + marks + ")");
    bindEntry(q, e);
    if (!q.exec()) {
        qWarning() << "KExamListDBTableHandler::AddExamEntity:" << q.lastError().text();
        return -1;
    }
    return 0;
}

int KExamListDBTableHandler::GetExamEntity(const std::string &examId, KExamEntry &out)
{
    QSqlQuery q(QSqlDatabase::database(g_conn));
    q.prepare("SELECT " + refColumnList() + " FROM tb_ExamList WHERE ExamId=?");
    q.addBindValue(QString::fromStdString(examId));
    if (!q.exec() || !q.next())
        return -1;
    out = fromQuery(q);
    return 0;
}

int KExamListDBTableHandler::UpdateExamEntity(const std::string &examId, const KExamEntry &e)
{
    QSqlQuery q(QSqlDatabase::database(g_conn));
    QString sets;
    for (int i = 0; i < kRefColumnCount; ++i) {
        if (i) sets += ", ";
        sets += QLatin1String(kColumns[i].name);
        sets += "=?";
    }
    q.prepare("UPDATE tb_ExamList SET " + sets + " WHERE ExamId=?");
    bindEntry(q, e);
    q.addBindValue(QString::fromStdString(examId));
    if (!q.exec()) {
        qWarning() << "KExamListDBTableHandler::UpdateExamEntity:" << q.lastError().text();
        return -1;
    }
    // Реф. не различает «обновлено 0 строк» и успех — сохраняем поведение.
    return 0;
}
