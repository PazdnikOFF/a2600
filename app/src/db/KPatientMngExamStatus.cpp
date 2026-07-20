#include "db/KPatientMngExamStatus.h"

#include <QDir>
#include <QString>
#include <QStringList>

#include "db/KExamBussinessHandler.h"
#include "db/KExamListDBTableHandler.h"
#include "hal/KUsbDevice.h"
#include "kernel/KSystemLog.h"
#include "kernel/KThreadPoolMsg.h"

namespace {
// Реф.: фильтры перечислены отдельными литералами.
const char *const kImageFilters[] = {"*.jpg", "*.png", "*.bmp"};
const char *const kVideoFilters[] = {"*.avi", "*.mp4", "*.mkv", "*.flv"};
const int kMsgExamStatusChanged = 12040;
} // namespace

KPatientMngExamStatus *KPatientMngExamStatus::GetInstance()
{
    static KPatientMngExamStatus inst;
    return &inst;
}

int KPatientMngExamStatus::GetFiletypeNumFromPath(const QString &path,
                                                  const QStringList &filters)
{
    return QDir(path).entryInfoList(filters, QDir::NoFilter, QDir::NoSort).size();
}

void KPatientMngExamStatus::SaveExamState(const MainUiPatientInfo &info)
{
    LogPrintf("[APP][I]: ", "power off SaveExamState");
    const std::string id = KExamBussinessHandler::GetInstance()->ExamId();
    if (id.empty())
        return;
    SaveState(id);
    SaveOverExamPatientInfo(info);
}

void KPatientMngExamStatus::SaveState(const std::string &examId)
{
    KExamEntry entry;
    entry.ResetToInvalid();
    if (KExamListDBTableHandler::GetExamEntity(examId, entry) != 0)
        return;

    const QString dir = KUsbDevice::GetInstance()->GetUsbPath()
                      + QString::fromStdString(entry.RecordPath);

    QStringList imgF, vidF;
    for (const char *f : kImageFilters) imgF << QString::fromLatin1(f);
    for (const char *f : kVideoFilters) vidF << QString::fromLatin1(f);
    const int imgNum = GetFiletypeNumFromPath(dir, imgF);
    const int vidNum = GetFiletypeNumFromPath(dir, vidF);

    bool changed = false;
    if (entry.RecordImgNum != imgNum)   { entry.RecordImgNum = imgNum;   changed = true; }
    if (entry.RecordVideoNum != vidNum) { entry.RecordVideoNum = vidNum; changed = true; }

    // Реф.: "Eg" (осмотр идёт) → "--" (отчёта ещё нет).
    if (entry.ReportStatus == "Eg") {
        entry.ReportStatus = "--";
        changed = true;
    }
    if (!changed)
        return;

    LogPrintf("[APP][I]: ", "UpdateExamEntity imgnum:%d ,videonum:%d ", imgNum, vidNum);
    KExamListDBTableHandler::UpdateExamEntity(examId, entry);
    KThreadPoolMsg::PostMsgToUI(0, kMsgExamStatusChanged, 0, 0, std::shared_ptr<void>());
}

void KPatientMngExamStatus::SaveOverExamPatientInfo(const MainUiPatientInfo & /*info*/)
{
    // Реф.: KQuickInputPatientDbTableHandler / KQuickInputApplicantDbTableHandler
    // IsExistEntity → AddEntity/UpdateEntity с меткой "yyyy-MM-dd hh:mm:ss".
    // У нас таблицы быстрого ввода живут в KEntityQuickInput с другим API —
    // подключение отложено (см. PROGRESS §10).
}
