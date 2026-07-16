#pragma once

#include <string>

// Строковые/DICOM-утилиты БД-слоя (реф. KDbStringOperation, X-2600). Облегчённый
// двойник KPatientStringOperation с ТЕМИ ЖЕ строковыми методами. Имя вводит в
// заблуждение: SQL-экранирования/кавычек/sanitize имён таблиц тут НЕТ.
// ВАЖНО: ConvertCharacterset/ToUTF8 здесь — ЗАГЛУШКИ `return true` (БД хранит UTF-8
// напрямую; вызываются из KDbSqlite::Exec/Query — DB-путь конверсию отключил).
// НЕ UI, НЕ device: чистый static-тулкит.
class KDbStringOperation
{
public:
    static void StringReplace(std::string &s, const std::string &from, const std::string &to);
    static void StringTrim(std::string &s);   // фикс. набор " \r\n\t"
    static void ReplaceInvalidCharInFolderName(std::string &s, const std::string &repl);

    static bool ConvertCharacterset(const std::string &from, const std::string &to,
                                    std::string &io);   // заглушка → true (no-op)
    static bool ConvertCharactersetToUTF8(std::string &io);   // заглушка → true (no-op)

    // Урезанная таблица (реф. меньше веток, чем у KPatient-версии); out — флаг распознавания.
    static std::string GetISOCharactersetOfDicom(const std::string &term, bool &out);

    static void SplitDicomPatientName(const std::string &src, std::string &p2,
                                      std::string &p3, std::string &p4);
    static std::string AssembleDicomPatientName(const std::string &a, const std::string &b,
                                                const std::string &c);
    static std::string AssembleCoverFilePath(const std::string &dir, const std::string &name);
};
