#pragma once

#include <QDate>
#include <QString>
#include <QStringList>

// Карточка отчёта для редактора (реф. report_edit::KReportItem, X-2600).
//
// РЕВЕРС: непеременная агрегатная структура — ни vtable, ни typeinfo; ctor
// подставляется inline (символа нет), эмитируется только деструктор
// (_ZN11report_edit11KReportItemD1Ev @0x4de388, weak ⇒ определена в заголовке),
// который разрушает 23 QString + 1 QStringList. **sizeof == 0xf0 (240)**.
//
// ⚠️ ЛОЖНЫЙ ДРУГ: `report/KReportTemplate.h` содержит `struct ReportItem` —
// это СОВСЕМ ДРУГОЕ (узел дерева шаблона: name/title/type/dataSrc/children).
// Объединять их НЕЛЬЗЯ.
//
// Имена полей ВЫВЕДЕНЫ из колонок-источников (ConvertToMap/NVP-тегов у этого
// типа нет), смещения — из дизасма.
namespace report_edit {

struct KReportItem {
    QString     RecordPath;       // 0x00 — usbPath + KExamEntry.RecordPath
    QString     ExamId;           // 0x08 — из аргумента, не из БД
    QDate       ExamDate;         // 0x10
    QString     Device;           // 0x18
    QString     ReportStatus;     // 0x20
    QString     PatientName;      // 0x28
    int         PatientSex = 0;   // 0x30 — через LoadDbInt, с доп. проверкой <= 3
    int         PatientAge = 0;   // 0x34
    QString     PatientID;        // 0x38
    QString     Applicants;       // 0x40
    QDate       PatientBirthday;  // 0x48
    QString     TelephoneNumber;  // 0x50
    QString     SickBedId;        // 0x58
    QString     UserItem1;        // 0x60
    QString     UserItem2;        // 0x68
    QString     ExamFindings;     // 0x70
    QString     Diagnose;         // 0x78
    QString     DiseaseName;      // 0x80
    QString     SurgicalMethod;   // 0x88
    QString     SurgeryFinding;   // 0x90
    QString     Suggest;          // 0x98
    QString     CustomField1;     // 0xa0
    QString     CustomField2;     // 0xa8
    QString     Biopsy;           // 0xb0
    int         HPType = 0;       // 0xb8
    QString     AssistDoctor;     // 0xc0
    QString     DrReportName;     // 0xc8
    QStringList ExamImg;          // 0xd0 — ChangeStringToFileList(KReportEntity.ExamImg)
    QString     TriofReference;   // 0xd8 — ВСЕГДА tr("TR_TRIOFReference"), безусловно
    int         ExamType = 0;     // 0xe0
    QDate       ReportDate;       // 0xe8
};

} // namespace report_edit
