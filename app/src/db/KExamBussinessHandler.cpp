#include "db/KExamBussinessHandler.h"

#include <QDir>
#include <QString>

#include <thread>

#include "db/KExamListDBTableHandler.h"
#include "db/KExamNoGenerate.h"
#include "db/KExamDataFileNameGenerator.h"
#include "db/KPatientMngExamStatus.h"
#include "dicom/KDicomInterface.h"
#include "hal/KUsbDevice.h"
#include "kernel/KSystemLog.h"
#include "kernel/KThreadPoolMsg.h"
#include "sys/KEncStyle.h"
#include "sys/KSystemStatus.h"
#include "sys/KTimeInfo.h"
#include <memory>   // std::shared_ptr/unique_ptr (libstdc++ не тянет транзитивно)

namespace {

// Стенды device-геттеров реф. (см. заголовок): KEndoScope::GetEndoInfo /
// KCamera::GetCameraInfo возвращают структуру с моделью (+0x00) и SN (+0x08).
std::string g_endoModel, g_endoSn, g_cameraModel, g_cameraSn;

// Реф. i18n идёт через QMetaObject::tr → QString::toUtf8 → std::string.
// Off-device словаря нет, поэтому ключ возвращается как есть (это и есть
// поведение Qt при отсутствии перевода).
std::string tr_(const char *key) { return std::string(key); }

// Сообщения UI (KThreadPoolMsg::PostMsgToUI), коды из реверса.
const int kMsgExamCreated   = 12041;  // 0x2f09
const int kMsgDataPathRenamed = 12044;

} // namespace

void KExamBussinessHandler::SetEndoInfo(const std::string &model, const std::string &sn)
{
    g_endoModel = model;
    g_endoSn = sn;
}

void KExamBussinessHandler::SetCameraInfo(const std::string &model, const std::string &sn)
{
    g_cameraModel = model;
    g_cameraSn = sn;
}

// --- синглтон ---------------------------------------------------------------

KExamBussinessHandler *KExamBussinessHandler::GetInstance()
{
    static std::once_flag once;
    static std::shared_ptr<KExamBussinessHandler> m_instance;
    std::call_once(once, [] {
        m_instance = std::shared_ptr<KExamBussinessHandler>(
            new KExamBussinessHandler(), [](KExamBussinessHandler *p) { delete p; });
    });
    return m_instance.get();
}

KExamBussinessHandler::KExamBussinessHandler()
{
    // Реф.: строки обеих встроенных записей = "INVALID_STRING", все int = -1,
    // m_eState = 1, m_bExaming = false; затем KPatientMngExamStatus::GetInstance()
    // (результат отбрасывается — только форсирует порядок конструирования)
    // и ClearData().
    m_examEntry.ResetToInvalid();
    KPatientMngExamStatus::GetInstance();
    ClearData();
}

// --- номер осмотра ----------------------------------------------------------

int KExamBussinessHandler::CreateTemporaryExamId()
{
    m_strTempExamId = KExamNoGenerate::MakeExamId();
    LogPrintf("[APP][I]: ", "Temporary exam id: %s", m_strTempExamId.c_str());
    return 0;   // реф. — всегда 0
}

int KExamBussinessHandler::TakeEffectExamId()
{
    KExamNoGenerate::SetExamId();          // коммит счётчика на диск
    m_strExamId = m_strTempExamId;
    m_mainUiPatientInfo.ExamId = m_strTempExamId;
    return 0;
}

bool KExamBussinessHandler::IsCurrentExamIdExaming(const std::string &examId)
{
    // Реф.: только сравнение строк, m_eState не участвует.
    return examId == m_mainUiPatientInfo.ExamId;
}

bool KExamBussinessHandler::IsExamEnd(const std::string &examId)
{
    if (!IsCurrentExamIdExaming(examId))
        return true;                       // чужой осмотр всегда «завершён»
    return !m_bExaming.load(std::memory_order_acquire);
}

// --- действия ---------------------------------------------------------------

int KExamBussinessHandler::EndoPowerOnAction()
{
    m_eState.store(2, std::memory_order_release);
    ClearData();
    if (CreateTemporaryExamId() != 0) {
        LogPrintf("[APP][I]: ", "Create temporary exam id failed");
        return -1;                          // реф.: недостижимо (см. выше)
    }
    return 0;
}

int KExamBussinessHandler::StartSaveDataAction()
{
    if (m_bExaming.load(std::memory_order_acquire))
        return 0;                           // уже идёт осмотр — no-op
    m_eState.store(3, std::memory_order_release);
    return 0;
}

int KExamBussinessHandler::EndoTypeChangeAction()
{
    return 0;                               // реф. — пустая заглушка
}

int KExamBussinessHandler::FinishSaveDataAction()
{
    if (!m_bExaming.load(std::memory_order_acquire)) {
        const long long t0 = GetMs();

        // 2. одноразовая защёлка
        m_eState.store(4, std::memory_order_release);      // E_EXAM_STATE_EXAMING
        m_bExaming.store(true, std::memory_order_release);

        // 3. номер вступает в силу
        TakeEffectExamId();

        // 4. анонимный фолбэк: PatientID := ExamId
        if (m_mainUiPatientInfo.PatientID.empty())
            m_mainUiPatientInfo.PatientID = m_mainUiPatientInfo.ExamId;

        // 5-6. локальная запись из карточки главного экрана (под мьютексом)
        KExamEntry entry;
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            entry.PatientName     = m_mainUiPatientInfo.PatientName;
            entry.PatientAge      = m_mainUiPatientInfo.PatientAge;
            entry.PatientSex      = m_mainUiPatientInfo.PatientSex;
            entry.PatientBirthday = m_mainUiPatientInfo.PatientBirthday;
            entry.Applicants      = m_mainUiPatientInfo.Applicants;
            entry.PatientID       = m_mainUiPatientInfo.PatientID;
            entry.UserItem1       = m_mainUiPatientInfo.UserItem1;
            entry.UserItem2       = m_mainUiPatientInfo.UserItem2;
            entry.ExamId          = m_mainUiPatientInfo.ExamId;
            entry.DrExamName      = m_mainUiPatientInfo.DrExamName;
            entry.ReportStatus    = "Eg";   // строка-enum реф.
        }

        // 7. поля из карточки пациента (вне мьютекса)
        entry.SickBedId       = m_patient.sickBedId.toStdString();
        entry.ApplicantDate   = m_patient.applicantDate.toStdString();
        entry.PlanDate        = m_patient.planDate.toStdString();
        entry.TelephoneNumber = m_patient.telephoneNumber.toStdString();
        entry.RegisterNumber  = m_patient.registerNumber.toStdString();
        entry.WorklistUID     = m_patient.worklistUID.toStdString();
        {
            bool ok = false;
            const int it = m_patient.examType.toInt(&ok);
            if (ok && it != -1)             // реф.: `if (ExamType != -1)`
                entry.ExamType = it;
        }

        // 8. PK пациента из tb_PatientList (реф. std::stoll, база 10)
        if (!m_strPatientListKey.empty()) {
            try {
                entry.PatientListTableKey = int(std::stoll(m_strPatientListKey, nullptr, 10));
            } catch (const std::exception &) {
                // реф. — исключение улетает наружу; у нас оставляем -1
            }
        }

        // 9. дата/время/каталог
        const KTimeInfo tm;                 // реф.: снимок времени, ctor → Init()
        entry.ExamDate   = tm.GetCurrentTimeYYYYMMDD("-");
        entry.ExamTime   = tm.GetCurrentTimeHHMMSS(":");
        entry.RecordPath = GetSaveDataPath();

        // 10. модель/серийник источника видео: ViewType 0 — эндоскоп, иначе камера
        const bool useCamera = KSystemStatus::GetInstance().ViewType() != 0;
        const std::string model = useCamera ? g_cameraModel : g_endoModel;
        entry.Device   = KEncStyle().GetEndoDisplayModel(QString::fromStdString(model))
                             .toStdString();
        entry.DeviceSN = useCamera ? g_cameraSn : g_endoSn;

        // 11. вставка
        if (KExamListDBTableHandler::AddExamEntity(entry) != 0) {
            // КВИРК реф.: тег [I], не [E]
            LogPrintf("[APP][I]: ", "KExamListDBTableHandler::AddExamEntity failed");
        } else {
            LogPrintf("[APP][I]: ", " Create exam info Use time %lld ms",
                      (long long)(GetMs() - t0));

            // 12. отдельный detached-поток на DICOM ActivateSeries
            const std::string uid = m_patient.worklistUID.toStdString();
            const std::string examId = m_strExamId;
            std::thread([uid, examId] {
                LogPrintf("[APP][I]: ", "First save Data --------ActivateSeries begin--------");
                KDicomInterface::GetInstance().ActivateSeries(uid, examId);
                LogPrintf("[APP][I]: ", "First save Data --------ActivateSeries end--------");
            }).detach();

            // 13. уведомление UI
            KThreadPoolMsg::PostMsgToUI(0, kMsgExamCreated, 0, 0, std::shared_ptr<void>());
        }
    }

    // Хвост — на ОБОИХ путях.
    KDicomInterface::GetInstance().DicomStore(m_strExamId);
    return 0;   // реф.: 0 даже при провале AddExamEntity
}

int KExamBussinessHandler::EndoPowerOffAction()
{
    if (m_bExaming.load(std::memory_order_acquire)) {
        UpdateExamDataPathName();
        const std::string examId = m_strExamId;
        std::thread([examId] {
            LogPrintf("[APP][I]: ", "EndoPowerOffAction --------EndSeries begin--------");
            KDicomInterface::GetInstance().EndSeries(examId);
            LogPrintf("[APP][I]: ", "EndoPowerOffAction --------EndSeries end----------");
        }).detach();
    }
    m_bExaming.store(false, std::memory_order_release);
    m_eState.store(5, std::memory_order_release);
    Generator().ResetData();
    KPatientMngExamStatus::GetInstance()->SaveExamState(m_mainUiPatientInfo);
    return 0;
}

// --- данные -----------------------------------------------------------------

void KExamBussinessHandler::ClearData()
{
    // Реф.: MainUiPatientInfo — строки в "", PatientAge = -1,
    // PatientSex = std::to_string(2) == "2" (а НЕ пусто!).
    m_mainUiPatientInfo.PatientName.clear();
    m_mainUiPatientInfo.PatientAge = -1;
    m_mainUiPatientInfo.PatientSex = std::to_string(2);
    m_mainUiPatientInfo.PatientBirthday.clear();
    m_mainUiPatientInfo.Applicants.clear();
    m_mainUiPatientInfo.PatientID.clear();
    m_mainUiPatientInfo.UserItem1.clear();
    m_mainUiPatientInfo.UserItem2.clear();
    m_mainUiPatientInfo.ExamId.clear();
    m_mainUiPatientInfo.DrExamName.clear();

    // Реф.: обе встроенные записи — строки в "INVALID_STRING", int в -1.
    m_examEntry.ResetToInvalid();
    const QString inv = QString::fromLatin1(exambiz::kInvalidString);
    m_patient = KPatientEntry();
    m_patient.id = inv;              m_patient.patientID = inv;
    m_patient.patientName = inv;     m_patient.patientSex = inv;
    m_patient.patientBirthday = inv; m_patient.applicantDate = inv;
    m_patient.applicants = inv;      m_patient.planDate = inv;
    m_patient.userItem1 = inv;       m_patient.userItem2 = inv;
    m_patient.sickBedId = inv;       m_patient.telephoneNumber = inv;
    m_patient.registerNumber = inv;  m_patient.worklistUID = inv;
    m_patient.patientAge = QStringLiteral("-1");
    m_patient.examStatus = QStringLiteral("-1");
    m_patient.examType   = QStringLiteral("-1");

    m_strPatientListKey.clear();
    m_strDataPath.clear();
    m_strTempExamId.clear();
    m_strExamId.clear();
    // Реф.: m_eState / m_bExaming / мьютекс НЕ трогаются, логов нет.
}

int KExamBussinessHandler::UpdateExamDataPathName()
{
    const std::string needle = m_strTempExamId + "-";
    const size_t pos = m_strDataPath.find(needle);
    if (pos == std::string::npos) {
        LogPrintfEx(true, "[APP][E]: ",
                    "UpdateExamDataPathName do not find %s in current path name",
                    m_strTempExamId.c_str());
        return 0;
    }
    std::string name = m_mainUiPatientInfo.PatientName;
    if (name.empty())
        name = "Anonymity";

    const std::string oldPath = m_strDataPath;
    const std::string newPath = m_strDataPath.substr(0, pos) + m_strTempExamId + "-" + name;

    // Реф.: QDir строится от СТАРОГО ПОЛНОГО пути (не от родителя); оба аргумента
    // абсолютные, поэтому rename всё равно срабатывает.
    QDir dir(QString::fromStdString(oldPath));
    if (dir.rename(QString::fromStdString(oldPath), QString::fromStdString(newPath))) {
        LogPrintf("[APP][I]: ", "rename from %s to %s success",
                  oldPath.c_str(), newPath.c_str());
        m_examEntry.RecordPath = newPath;
        KExamListDBTableHandler::UpdateExamEntity(m_mainUiPatientInfo.ExamId, m_examEntry);
        m_strDataPath = newPath;
        KThreadPoolMsg::PostMsgToUI(0, kMsgDataPathRenamed, 0, 0, std::shared_ptr<void>());
    } else {
        // Реф.: при неудаче состояние НЕ меняется вообще.
        LogPrintfEx(true, "[APP][E]: ", "rename from %s to %s failed",
                    oldPath.c_str(), newPath.c_str());
    }
    return 0;
}

std::string KExamBussinessHandler::GetSaveDataPath()
{
    if (m_strDataPath.empty()) {
        LogPrintfEx(true, "[APP][E]: ", "current store file path is empty!");
        return std::string();
    }
    const std::string usb = KUsbDevice::GetInstance()->GetUsbPath().toStdString();
    const size_t pos = usb.empty() ? std::string::npos : m_strDataPath.find(usb);
    if (pos == std::string::npos) {
        LogPrintfEx(true, "[APP][E]: ",
                    "full store file path is not include current udisk path !");
        return m_strDataPath;   // КВИРК реф.: возвращает ПОЛНЫЙ путь
    }
    return m_strDataPath.substr(pos + usb.size());
}

// --- карточка пациента ------------------------------------------------------

int KExamBussinessHandler::UpdatePatientInfoFromMainUi(const MainUiPatientInfo &info)
{
    std::lock_guard<std::mutex> lk(m_mutex);
    m_mainUiPatientInfo = info;   // реф.: покомпонентное копирование всех 10 полей
    return 0;
}

int KExamBussinessHandler::UpdatePatientInfoFrmoDB(const KPatientEntry &e)
{
    // Реф.: 7 строк + PatientAge; ExamId и DrExamName НЕ трогаются.
    // КВИРК: мьютекс НЕ берётся (в отличие от соседних методов).
    m_mainUiPatientInfo.PatientName     = e.patientName.toStdString();
    m_mainUiPatientInfo.PatientSex      = e.patientSex.toStdString();
    m_mainUiPatientInfo.PatientBirthday = e.patientBirthday.toStdString();
    m_mainUiPatientInfo.Applicants      = e.applicants.toStdString();
    m_mainUiPatientInfo.PatientID       = e.patientID.toStdString();
    m_mainUiPatientInfo.UserItem1       = e.userItem1.toStdString();
    m_mainUiPatientInfo.UserItem2       = e.userItem2.toStdString();
    m_mainUiPatientInfo.PatientAge      = e.patientAge.toInt();
    return 0;
}

int KExamBussinessHandler::UpdatePatientInfoFromReport(const std::string &examId)
{
    const long long t0 = GetMs();
    if (!IsCurrentExamIdExaming(examId)) {
        LogPrintf("[APP][I]: ", "Exam ID:[%s] is not current exam id, nothing to do",
                  examId.c_str());
        return 0;
    }
    // Реф.: ключ БД — КЭШИРОВАННЫЙ id, а не аргумент.
    if (KExamListDBTableHandler::GetExamEntity(m_mainUiPatientInfo.ExamId, m_examEntry) != 0) {
        LogPrintfEx(true, "[APP][E]: ", "Get examlist info form db failed");
        return -1;
    }
    m_mainUiPatientInfo.PatientName     = m_examEntry.PatientName;
    m_mainUiPatientInfo.PatientAge      = m_examEntry.PatientAge;
    m_mainUiPatientInfo.PatientSex      = m_examEntry.PatientSex;
    m_mainUiPatientInfo.PatientBirthday = m_examEntry.PatientBirthday;
    m_mainUiPatientInfo.Applicants      = m_examEntry.Applicants;
    m_mainUiPatientInfo.PatientID       = m_examEntry.PatientID;
    m_mainUiPatientInfo.UserItem1       = m_examEntry.UserItem1;
    m_mainUiPatientInfo.UserItem2       = m_examEntry.UserItem2;
    m_mainUiPatientInfo.ExamId          = m_examEntry.ExamId;
    m_mainUiPatientInfo.DrExamName      = m_examEntry.DrExamName;
    LogPrintfEx(false, "[APP][D]: ", " Update Patient Info From Report Use time %lld ms",
                (long long)(GetMs() - t0));
    return 0;
}

int KExamBussinessHandler::SaveMainUiPatientInfo(const MainUiPatientInfo &info)
{
    UpdatePatientInfoFromMainUi(info);
    if (info.ExamId.empty()) {
        if (m_eState.load() == 4)
            LogPrintfEx(true, "[APP][E]: ",
                        "Exam State[E_EXAM_STATE_EXAMING] Exam ID is empty, an error occured!");
        else
            LogPrintfEx(false, "[APP][D]: ", "Exam State[%d] Exam ID is empty, nothing to do",
                        m_eState.load());
        return 0;
    }
    UpdateExamListTable([&info, this](KExamEntry &e) {
        std::lock_guard<std::mutex> lk(m_mutex);
        e.PatientAge      = info.PatientAge;
        e.Applicants      = info.Applicants;
        e.PatientBirthday = info.PatientBirthday;
        e.ExamId          = info.ExamId;
        e.PatientID       = info.PatientID;
        e.PatientName     = info.PatientName;
        e.PatientSex      = info.PatientSex;
        e.UserItem1       = info.UserItem1;
        e.UserItem2       = info.UserItem2;
        // Реф.: пока врач отчёта совпадает с врачом осмотра — тянем его следом;
        // как только пользователь его переопределил — больше не трогаем.
        if (e.DrReportName == e.DrExamName)
            e.DrReportName = info.DrExamName;
        e.DrExamName = info.DrExamName;
    });
    return 0;   // реф.: всегда 0
}

// --- триггер осмотра из списка пациентов ------------------------------------

int KExamBussinessHandler::PatientListTriggerExamAction(const KPatientEntry &e)
{
    return TriggerExamAction(e);   // реф.: чистый tail-jump
}

int KExamBussinessHandler::PatientListTriggerExamAction(const std::string &patientId)
{
    KPatientEntry entry;
    entry.patientAge = QStringLiteral("-1");
    entry.examType   = QStringLiteral("-1");
    entry.examStatus = QStringLiteral("-1");
    // Реф. работает с единственным соединением; у нас оно задаётся
    // KExamListDBTableHandler::SetConnectionName (одна БД на обе таблицы).
    KPatientListDBTableHandler db(KExamListDBTableHandler::ConnectionName());
    if (db.GetEntity(QString::fromStdString(patientId), entry) != 0) {
        LogPrintfEx(true, "[APP][E]: ", "Get patient info form db failed");
        return -1;
    }
    m_strPatientListKey = patientId;
    return TriggerExamAction(entry);
}

int KExamBussinessHandler::TriggerExamAction(const KPatientEntry &pat)
{
    m_patient = pat;                       // реф.: 1:1 все 16 полей, включая ExamStatus
    UpdatePatientInfoFrmoDB(m_patient);

    if (!m_bExaming.load(std::memory_order_acquire))
        return 0;                          // осмотр ещё не начат — в БД не пишем

    UpdateExamListTable([this](KExamEntry &e) {
        std::lock_guard<std::mutex> lk(m_mutex);
        // Реф.: ExamStatus НАМЕРЕННО не переносится.
        e.PatientName     = m_patient.patientName.toStdString();
        e.PatientSex      = m_patient.patientSex.toStdString();
        e.PatientAge      = m_patient.patientAge.toInt();
        e.PatientBirthday = m_patient.patientBirthday.toStdString();
        e.PatientID       = m_patient.patientID.toStdString();
        e.ExamType        = m_patient.examType.toInt();
        e.ApplicantDate   = m_patient.applicantDate.toStdString();
        e.Applicants      = m_patient.applicants.toStdString();
        e.PlanDate        = m_patient.planDate.toStdString();
        e.SickBedId       = m_patient.sickBedId.toStdString();
        e.TelephoneNumber = m_patient.telephoneNumber.toStdString();
        e.UserItem1       = m_patient.userItem1.toStdString();
        e.UserItem2       = m_patient.userItem2.toStdString();
        e.WorklistUID     = m_patient.worklistUID.toStdString();
        e.RegisterNumber  = m_patient.registerNumber.toStdString();
    });
    KDicomInterface::GetInstance().RebindWorklist(m_strExamId,
                                                 m_patient.worklistUID.toStdString());
    return 0;   // реф.: ошибки БД проглатываются
}

int KExamBussinessHandler::UpdateExamListTable(const std::function<void(KExamEntry &)> &fn)
{
    const long long t0 = GetMs();
    if (KExamListDBTableHandler::GetExamEntity(m_mainUiPatientInfo.ExamId, m_examEntry) != 0) {
        LogPrintfEx(true, "[APP][E]: ", "Get examlist info form db failed");
        return -1;
    }
    if (!fn)
        throw std::bad_function_call();    // реф.: std::__throw_bad_function_call()
    fn(m_examEntry);
    if (KExamListDBTableHandler::UpdateExamEntity(m_mainUiPatientInfo.ExamId, m_examEntry) != 0) {
        LogPrintfEx(true, "[APP][E]: ", "UpdateExamEntity failed");
        return -1;
    }
    LogPrintf("[APP][I]: ", " Update ExamList Table Use time %lld ms",
              (long long)(GetMs() - t0));
    return 0;
}

// --- пол --------------------------------------------------------------------

std::string KExamBussinessHandler::GetGender(const std::string &sex)
{
    // ВОСПРОИЗВЕДЕНИЕ БАГА ОРИГИНАЛА (см. заголовок): в X2000 скомпилировано
    //     if (sex.compare(to_string(1))) return tr("TR_M");
    //     if (sex.compare(to_string(0))) return tr("TR_F");
    //     return tr("TR_Nknwn");
    // то есть ветка берётся при НЕравенстве. Итог: "1" → TR_F; "0" → TR_M;
    // всё прочее → TR_M; TR_Nknwn недостижим.
    if (sex.compare(std::to_string(1)) != 0)
        return tr_("TR_M");
    if (sex.compare(std::to_string(0)) != 0)
        return tr_("TR_F");
    return tr_("TR_Nknwn");   // мёртвый код (как в оригинале)
}
