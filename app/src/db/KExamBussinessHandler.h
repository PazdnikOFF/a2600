#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include "db/KExamEntry.h"
#include "db/KEntityPatient.h"   // KPatientEntry

// Оркестратор жизненного цикла осмотра (реф. KExamBussinessHandler, X-2600).
// Опечатка «Bussiness» — из оригинала, сохранена намеренно.
//
// РЕВЕРС: непеременный (non-polymorphic) синглтон, sizeof == 0x720; НЕ QObject,
// НЕ QDialog — сигналов/слотов нет вообще. Все методы возвращают int (0 — ок),
// кроме явно помеченных. Клей поверх KEntityPatient/KExamListDBTableHandler.
//
// Состояния m_eState (реф. значения): 1 — ctor/idle, 2 — power-on,
// 3 — saving-started, 4 — E_EXAM_STATE_EXAMING, 5 — powered-off.
class KExamBussinessHandler
{
public:
    // Реф.: std::call_once + shared_ptr, наружу отдаётся СЫРОЙ указатель.
    static KExamBussinessHandler *GetInstance();

    // --- жизненный цикл номера осмотра -------------------------------------
    // Временный номер: KExamNoGenerate::MakeExamId() БЕЗ коммита счётчика.
    int  CreateTemporaryExamId();
    // Коммит: KExamNoGenerate::SetExamId() + m_strExamId = m_strTempExamId
    // и MainUiPatientInfo.ExamId = m_strTempExamId.
    int  TakeEffectExamId();
    // Реф.: ЧИСТОЕ сравнение строк с MainUiPatientInfo.ExamId; m_eState НЕ читает
    // (несмотря на имя).
    bool IsCurrentExamIdExaming(const std::string &examId);
    // Реф.: чужой ExamId всегда «завершён» (true); свой — !m_bExaming.
    bool IsExamEnd(const std::string &examId);

    // --- действия ----------------------------------------------------------
    int  EndoPowerOnAction();     // m_eState=2, ClearData, CreateTemporaryExamId
    int  EndoPowerOffAction();    // rename каталога + EndSeries, m_eState=5
    int  StartSaveDataAction();   // no-op если уже m_bExaming, иначе m_eState=3
    int  FinishSaveDataAction();  // главный: фиксация номера + вставка в БД
    int  EndoTypeChangeAction();  // реф. — ПУСТАЯ заглушка `return 0`

    // --- данные ------------------------------------------------------------
    void ClearData();
    // Реф.: <parent>/<tempExamId>-<PatientName|"Anonymity">; при неудаче rename
    // состояние НЕ меняется.
    int  UpdateExamDataPathName();
    // Реф.: срезает префикс пути USB-накопителя из m_strDataPath.
    // КВИРК: если префикс не найден — возвращает ПОЛНЫЙ путь (неотличимо от успеха).
    std::string GetSaveDataPath();

    int  UpdatePatientInfoFromMainUi(const MainUiPatientInfo &info);
    // Опечатка «Frmo» — из оригинала. Реф.: БЕЗ мьютекса (в отличие от соседей).
    int  UpdatePatientInfoFrmoDB(const KPatientEntry &e);
    int  UpdatePatientInfoFromReport(const std::string &examId);
    int  SaveMainUiPatientInfo(const MainUiPatientInfo &info);

    int  PatientListTriggerExamAction(const std::string &patientId);
    int  PatientListTriggerExamAction(const KPatientEntry &e);  // реф.: alias TriggerExamAction
    int  TriggerExamAction(const KPatientEntry &e);

    // Read-modify-write осмотра по MainUiPatientInfo.ExamId. -1 при любой ошибке БД.
    int  UpdateExamListTable(const std::function<void(KExamEntry &)> &fn);

    // Реф. GetGender(const KString&) — ВНИМАНИЕ, В ОРИГИНАЛЕ БАГ ПОЛЯРНОСТИ:
    // код скомпилирован как `if (sex.compare("1")) return tr("TR_M");`
    // (пропущено `== 0`), поэтому "1" → TR_F, всё остальное → TR_M,
    // а TR_Nknwn недостижим. Баг ВОСПРОИЗВЕДЁН 1:1 (см. .cpp).
    static std::string GetGender(const std::string &sex);

    // --- доступ к состоянию (не из реф.; для self-test и оркестрации) -------
    int  ExamState() const           { return m_eState.load(); }
    bool IsExaming() const           { return m_bExaming.load(); }
    const std::string &TempExamId() const { return m_strTempExamId; }
    const std::string &ExamId() const     { return m_strExamId; }
    const std::string &DataPath() const   { return m_strDataPath; }
    void SetDataPath(const std::string &p) { m_strDataPath = p; }
    const MainUiPatientInfo &MainUiInfo() const { return m_mainUiPatientInfo; }
    const KExamEntry &ExamEntry() const   { return m_examEntry; }

    // Стенды вместо device-геттеров реф. (KEndoScope::GetEndoInfo /
    // KCamera::GetCameraInfo → поля +0x00 модель, +0x08 серийник).
    static void SetEndoInfo(const std::string &model, const std::string &sn);
    static void SetCameraInfo(const std::string &model, const std::string &sn);

private:
    KExamBussinessHandler();
    ~KExamBussinessHandler() = default;
    KExamBussinessHandler(const KExamBussinessHandler &) = delete;
    KExamBussinessHandler &operator=(const KExamBussinessHandler &) = delete;

    std::string       m_strDataPath;        // 0x000
    std::atomic<int>  m_eState{1};          // 0x020
    std::string       m_strTempExamId;      // 0x028
    std::string       m_strExamId;          // 0x048
    std::atomic<bool> m_bExaming{false};    // 0x068
    KPatientEntry     m_patient;            // 0x070
    KExamEntry        m_examEntry;          // 0x220
    std::string       m_strPatientListKey;  // 0x5c8 — PK пациента (десятичная строка)
    MainUiPatientInfo m_mainUiPatientInfo;  // 0x5e8
    mutable std::mutex m_mutex;             // 0x710 — реф. KMutex; ЗАЩИЩАЕТ ТОЛЬКО
                                            //         блок MainUiPatientInfo
};
