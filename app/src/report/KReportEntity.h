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

// 15 колонок, имена ВЕРБАТИМ из реверса. Единственная целочисленная — HPType.
struct KReportEntity {
    std::string ExamId;          // главный ключ
    std::string ReportDate;      // формат "yyyy-MM-dd"
    std::string ExamFindings;
    std::string DiseaseName;
    std::string SurgicalMethod;
    std::string SurgeryFinding;
    std::string Biopsy;
    std::string CustomField1;
    std::string CustomField2;
    std::string Suggest;
    std::string AssistDoctor;
    std::string ExamImg;         // ЕДИНСТВЕННАЯ колонка через ChangeFileListToString
    std::string Reserved1;
    std::string Reserved2;
    int         HPType = -1;     // отображается как TR_Ngtv(-)/TR_Pstv(+)

    // Реф. ConvertToMap/ConvertToEntry — обмен с KEntityManage идёт картой.
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
