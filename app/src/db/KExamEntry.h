#pragma once

#include <string>

// Сущности осмотра и «живой» карточки пациента главного экрана (реф. X-2600).
//
// РЕВЕРС: раскладка и ИМЕНА полей взяты 1:1 из строковых литералов колонок в
// KExamEntry::ConvertToMap / KPatientEntry::ConvertToMap бинарника X2000, порядок
// сохранён (см. смещения в комментариях — это офсеты в реф. структурах).
//
// В реф. все строки — std::string, целые — int с сентинелом -1 («не задано»,
// ConvertToMap такие поля ПРОПУСКАЕТ). Здесь то же самое: std::string/int,
// потому что KExamBussinessHandler оперирует именно ими (не QString).
//
// ВАЖНО (квирк реф.): конструктор KExamBussinessHandler и его ClearData()
// заполняют строковые поля KExamEntry/KPatientEntry НЕ пустой строкой, а
// файл-статическим сентинелом "INVALID_STRING" (см. kInvalidString ниже).
namespace exambiz { extern const char *const kInvalidString; }   // "INVALID_STRING"

// Реф. KExamEntry, sizeof == 0x3a8. Таблица tb_ExamList.
struct KExamEntry {
    int         PatientListTableKey = -1;  // 0x000 — PK пациента из tb_PatientList
    std::string PatientName;               // 0x008
    std::string PatientSex;                // 0x028
    int         PatientAge = -1;           // 0x048
    std::string PatientBirthday;           // 0x050
    std::string PatientID;                 // 0x070
    std::string ExamId;                    // 0x090 (ключ)
    std::string ExamDate;                  // 0x0b0
    std::string DrExamName;                // 0x0d0
    std::string DrExamId;                  // 0x0f0
    std::string DrExamRole;                // 0x110
    int         ExamType = -1;             // 0x130
    std::string Device;                    // 0x138
    std::string DeviceSN;                  // 0x158
    std::string DrReportName;              // 0x178
    std::string DrReportId;                // 0x198
    std::string DrReportRole;              // 0x1b8
    std::string ApplicantDate;             // 0x1d8
    std::string Applicants;                // 0x1f8
    std::string PlanDate;                  // 0x218
    std::string SickBedId;                 // 0x238
    std::string TelephoneNumber;           // 0x258
    std::string UserItem1;                 // 0x278
    std::string UserItem2;                 // 0x298
    std::string ReportStatus;              // 0x2b8 — строка-enum ("Eg" при создании)
    int         ReportPrintTimes = -1;     // 0x2d8
    int         UploadTimes = -1;          // 0x2dc
    std::string RecordPath;                // 0x2e0 — каталог данных осмотра
    std::string RegisterNumber;            // 0x300
    std::string Reserved1;                 // 0x320
    std::string Reserved2;                 // 0x340
    std::string WorklistUID;               // 0x360
    std::string ExamTime;                  // 0x380
    int         RecordImgNum = -1;         // 0x3a0
    int         RecordVideoNum = -1;       // 0x3a4

    // Сброс в состояние ctor/ClearData реф.: строки = "INVALID_STRING", int = -1.
    void ResetToInvalid();
};

// Реф. MainUiPatientInfo, sizeof == 0x128 — карточка пациента, редактируемая
// на главном экране (живое состояние UI, не запись БД).
struct MainUiPatientInfo {
    std::string PatientName;       // 0x000
    int         PatientAge = -1;   // 0x020
    std::string PatientSex;        // 0x028
    std::string PatientBirthday;   // 0x048
    std::string Applicants;        // 0x068
    std::string PatientID;         // 0x088
    std::string UserItem1;         // 0x0a8
    std::string UserItem2;         // 0x0c8
    std::string ExamId;            // 0x0e8
    std::string DrExamName;        // 0x108
};
