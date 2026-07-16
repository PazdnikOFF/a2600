#include "dicom/KDbStringOperation.h"
#include "dicom/KPatientStringOperation.h"

// Строковые методы идентичны KPatient-версии (реф. дублирует их) — делегируем,
// чтобы не расходиться в поведении.

void KDbStringOperation::StringReplace(std::string &s, const std::string &from,
                                       const std::string &to)
{
    KPatientStringOperation::StringReplace(s, from, to);
}

void KDbStringOperation::StringTrim(std::string &s)
{
    KPatientStringOperation::StringTrim(s);
}

void KDbStringOperation::ReplaceInvalidCharInFolderName(std::string &s, const std::string &repl)
{
    KPatientStringOperation::ReplaceInvalidCharInFolderName(s, repl);
}

bool KDbStringOperation::ConvertCharacterset(const std::string &, const std::string &,
                                             std::string &)
{
    return true;   // реф. — заглушка (БД хранит UTF-8 напрямую)
}

bool KDbStringOperation::ConvertCharactersetToUTF8(std::string &)
{
    return true;   // реф. — заглушка
}

std::string KDbStringOperation::GetISOCharactersetOfDicom(const std::string &term, bool &out)
{
    // Урезанная таблица реф. (видимые в дизасме ветки).
    struct Row { const char *term; const char *cs; };
    static const Row table[] = {
        {"ISO_IR 192", "utf-8"}, {"ISO_IR 144", "ISO-8859-5"},
        {"ISO_IR 100", "ISO-8859-1"}, {"GB18030", "GB2312"},
    };
    for (const Row &r : table) {
        if (term == r.term) {
            out = true;
            return r.cs;
        }
    }
    out = false;
    return std::string();
}

void KDbStringOperation::SplitDicomPatientName(const std::string &src, std::string &p2,
                                               std::string &p3, std::string &p4)
{
    KPatientStringOperation::SplitDicomPatientName(src, p2, p3, p4);
}

std::string KDbStringOperation::AssembleDicomPatientName(const std::string &a,
                                                         const std::string &b,
                                                         const std::string &c)
{
    return KPatientStringOperation::AssembleDicomPatientName(a, b, c);
}

std::string KDbStringOperation::AssembleCoverFilePath(const std::string &dir,
                                                      const std::string &name)
{
    return KPatientStringOperation::AssembleCoverFilePath(dir, name);
}
