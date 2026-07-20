#pragma once

#include <map>
#include <string>
#include <vector>

// Запись отчёта (реф. KReportEntity + KReportDBTableHandler, таблица **Report**).
//
// ⚠️ КОЛЛИЗИЯ С СУЩЕСТВУЮЩИМ КОДОМ — НЕ РАЗРЕШЕНА НАМЕРЕННО, нужна отдельная
// сверка (см. docs/PROGRESS.md §10):
//   • наш `report/KEntityReport.h` объявляет себя «реф. KEntityReport/
//     KReportDBTableHandler, tb_Report», но у него ДРУГАЯ таблица (`tb_Report`
//     против `Report`), ДРУГОЙ ключ (`accessionNumber` против `ExamId`) и
//     ДРУГИЕ колонки (есть templateName/examView, которых в реф. нет; нет
//     ExamFindings/CustomField1/2/AssistDoctor/ExamImg/Reserved1/2/HPType).
//   • совпадают по имени лишь GetAllRecordMainKey и GetQueryRecordNum, и у них
//     РАЗНЫЕ сигнатуры.
// Здесь заведён ФАКТИЧЕСКИЙ слой референса; KEntityReport НЕ ТРОГАЕТСЯ, чтобы
// не сломать существующие self-test (`reportdb` и др.). Дублирование временное.

// 16 колонок, имена и ПОРЯДОК (= смещения) ВЕРБАТИМ из KReportEntity::ConvertToMap
// (@0x429ef0). sizeof == 0x1e8. Единственная целочисленная — HPType.
//
// ИСПРАВЛЕНО ПО ВТОРОМУ ЗАХОДУ РЕВЕРСА (первая версия этого файла была неверной):
//   • колонка **Diagnose** (0x60) ОТСУТСТВОВАЛА. Её ключ в ConvertToMap — не литерал
//     в .rodata, а 8-байтовый непосредственный операнд "Diagnose", поэтому греп по
//     строкам его не находил;
//   • **HPType стоит на 0x160**, между Suggest и AssistDoctor, а не последним полем.
struct KReportEntity {
    std::string ExamId;          // 0x000 главный ключ
    std::string ReportDate;      // 0x020 формат "yyyy-MM-dd"
    std::string ExamFindings;    // 0x040
    std::string Diagnose;        // 0x060
    std::string DiseaseName;     // 0x080
    std::string SurgicalMethod;  // 0x0a0
    std::string SurgeryFinding;  // 0x0c0
    std::string Biopsy;          // 0x0e0
    std::string CustomField1;    // 0x100
    std::string CustomField2;    // 0x120
    std::string Suggest;         // 0x140
    int         HPType = -1;     // 0x160 — TR_Ngtv(-)/TR_Pstv(+)
    std::string AssistDoctor;    // 0x168
    std::string ExamImg;         // 0x188 ЕДИНСТВЕННАЯ колонка через ChangeFileListToString
    std::string Reserved1;       // 0x1a8
    std::string Reserved2;       // 0x1c8

    // Реф. ConvertToMap/ConvertToEntry — обмен с KEntityManage идёт картой.
    // КВИРК реф.: ConvertToMap ПРОПУСКАЕТ поля, равные "INVALID_STRING", и
    // пропускает HPType при значении -1.
    std::map<std::string, std::string> ConvertToMap() const;
    void ConvertToEntry(const std::map<std::string, std::string> &m);
};

// Тонкий статический фасад (реф. KReportDBTableHandler): ни базового класса,
// ни vtable, ни полей; ctor — голый `ret`. Статическое поле
// m_strEntityType == "Report"; все методы форвардят в KEntityManage с этим типом.
class KReportDBTableHandler
{
public:
    static const char *const m_strEntityType;   // "Report"
    // Реф. код «не поддерживается» для DeleteEntites.
    static const int ERR_NOT_SUPPORT = -8193;

    static bool CreateTable(const class QString &connectionName);

    static int GetRecordNumber();
    static int AddNewReportEntity(const KReportEntity &e);
    static int UpdateReportEntity(const std::string &examId, const KReportEntity &e);
    static int GetEntity(const std::string &examId, KReportEntity &out);
    static int DeleteEntity(const std::string &examId);
    // Реф.: НЕ РЕАЛИЗОВАН — возвращает ERR_NOT_SUPPORT.
    static int DeleteEntites(const std::vector<std::string> &ids);
    static int GetPageRecordFromDb(const std::map<std::string, std::string> &query,
                                   std::vector<std::map<std::string, std::string>> &out);
    static int GetQueryRecordNum(const std::map<std::string, std::string> &query);
    static int GetAllRecordMainKey(std::vector<std::string> &out);

    // Не из реф. — выбор соединения для off-device self-test.
    static void SetConnectionName(const class QString &connectionName);
};
