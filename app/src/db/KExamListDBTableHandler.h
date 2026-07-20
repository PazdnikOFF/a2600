#pragma once

#include <string>

#include "db/KExamEntry.h"

class QString;

// Хендлер таблицы осмотров (реф. KExamListDBTableHandler, tb_ExamList, X-2600).
//
// РЕВЕРС: в бинарнике X2000 Add/Get/UpdateExamEntity скомпилированы БЕЗ `this`
// (статические/свободные функции) — здесь тоже static. Код возврата: 0 — успех,
// НЕ 0 — ошибка (KExamBussinessHandler проверяет именно на !=0).
//
// СООТНОШЕНИЕ С KEntityExam (наш более ранний, «тонкий» слой над той же таблицей):
// KEntityExam знает 10 колонок (ExamId/AccessionNumber/PatientId/ExamType/ExamDate/
// ExamTime/ExamStatus/RegisterNumber/DrExamId/ExamDir), реф. KExamEntry — 35.
// Обе схемы уживаются: CreateTable здесь создаёт НАДМНОЖЕСТВО колонок, а если
// таблица уже создана «тонким» слоем — добирает недостающие через ALTER TABLE
// ADD COLUMN (SQLite: имена колонок регистронезависимы, поэтому PatientId реф.
// PatientID — ОДНА И ТА ЖЕ колонка).
class KExamListDBTableHandler
{
public:
    // Создать/дообновить схему tb_ExamList под полный набор колонок KExamEntry.
    static bool CreateTable(const QString &connectionName);

    // Реф.: 0 — успех.
    static int AddExamEntity(const KExamEntry &e);
    static int GetExamEntity(const std::string &examId, KExamEntry &out);
    static int UpdateExamEntity(const std::string &examId, const KExamEntry &e);

    // Не из реф. — точка настройки соединения для off-device self-test
    // (на устройстве соединение одно, endo_main).
    static void SetConnectionName(const QString &connectionName);
    static QString ConnectionName();
};
