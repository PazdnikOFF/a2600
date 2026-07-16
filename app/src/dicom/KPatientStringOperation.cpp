#include "dicom/KPatientStringOperation.h"

#include <QFile>

#include <iconv.h>

#include <vector>

namespace {
const char *TRIM_CHARS    = " \r\n\t";
const char *INVALID_CHARS = "\\/:*?\"<>|";
} // namespace

void KPatientStringOperation::StringReplace(std::string &s, const std::string &from,
                                            const std::string &to)
{
    if (from.empty())
        return;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
}

void KPatientStringOperation::StringTrim(std::string &s)
{
    const size_t b = s.find_first_not_of(TRIM_CHARS);
    if (b == std::string::npos) {
        s.clear();
        return;
    }
    const size_t e = s.find_last_not_of(TRIM_CHARS);
    s = s.substr(b, e - b + 1);
}

void KPatientStringOperation::ReplaceInvalidCharInFolderName(std::string &s,
                                                             const std::string &repl)
{
    // Реф.: каждый символ из набора → repl (find_first_of по всему набору, цикл).
    size_t pos = 0;
    while ((pos = s.find_first_of(INVALID_CHARS, pos)) != std::string::npos) {
        s.replace(pos, 1, repl);
        pos += repl.size();
    }
}

std::string KPatientStringOperation::GetSOPClassUID(int type, bool flag)
{
    switch (type) {
    case 0:
        return flag ? "1.2.840.10008.5.1.4.1.1.3.1"     // US Multiframe
                    : "1.2.840.10008.5.1.4.1.1.77.1.1"; // VL Endoscopic
    case 1:
        return "1.2.840.10008.5.1.4.1.1.7";             // Secondary Capture
    case 2:
        return "1.2.840.10008.5.1.4.1.1.6.2";           // US Image
    default:
        return std::string();
    }
}

bool KPatientStringOperation::ConvertCharacterset(const std::string &from, const std::string &to,
                                                  std::string &io)
{
    if (io.empty())
        return true;   // реф. no-op

    iconv_t cd = iconv_open(to.c_str(), from.c_str());
    if (cd == reinterpret_cast<iconv_t>(-1))
        return false;

    std::vector<char> inbuf(io.begin(), io.end());
    char *inptr = inbuf.data();
    size_t inleft = inbuf.size();

    std::vector<char> outbuf(5120, '\0');   // реф. фиксированный буфер 5120 → усечение
    char *outptr = outbuf.data();
    size_t outleft = outbuf.size();

    iconv(cd, &inptr, &inleft, &outptr, &outleft);
    iconv_close(cd);

    io.assign(outbuf.data(), outbuf.size() - outleft);
    return true;
}

bool KPatientStringOperation::ConvertCharactersetToUTF8(std::string &io)
{
    for (const char *src : {"GB2312", "ISO-8859-1", "ISO-8859-5"}) {
        std::string copy = io;
        if (ConvertCharacterset(src, "utf-8", copy)) {
            io = copy;
            return true;
        }
    }
    return false;
}

std::string KPatientStringOperation::GetISOCharactersetOfDicom(const std::string &term, bool &out)
{
    // Таблица DICOM defined-term → iconv-charset (реф. KPatient-версия, ~12 веток).
    struct Row { const char *term; const char *cs; };
    static const Row table[] = {
        {"ISO_IR 100", "ISO-8859-1"}, {"ISO_IR 101", "ISO-8859-2"},
        {"ISO_IR 109", "ISO-8859-3"}, {"ISO_IR 110", "ISO-8859-4"},
        {"ISO_IR 144", "ISO-8859-5"}, {"ISO_IR 127", "ISO-8859-6"},
        {"ISO_IR 126", "ISO-8859-7"}, {"ISO_IR 138", "ISO-8859-8"},
        {"ISO_IR 148", "ISO-8859-9"}, {"ISO_IR 192", "utf-8"},
        {"GB18030", "GB18030"},       {"GB2312", "GB2312"},
    };
    for (const Row &r : table) {
        if (term == r.term) {
            out = true;   // распознан (точная семантика флага в реф. не подтверждена)
            return r.cs;
        }
    }
    out = false;
    return std::string();
}

void KPatientStringOperation::SplitDicomPatientName(const std::string &src, std::string &p2,
                                                    std::string &p3, std::string &p4)
{
    // Реф.: без '^' → p2 = вся строка; с '^' → token0→p4, token1→p2, token2→p3.
    if (src.find('^') == std::string::npos) {
        p2 = src;
        return;
    }
    std::vector<std::string> toks;
    size_t start = 0;
    for (;;) {
        const size_t pos = src.find('^', start);
        if (pos == std::string::npos) {
            toks.push_back(src.substr(start));
            break;
        }
        toks.push_back(src.substr(start, pos - start));
        start = pos + 1;
    }
    if (toks.size() > 0) p4 = toks[0];
    if (toks.size() > 1) p2 = toks[1];
    if (toks.size() > 2) p3 = toks[2];
}

std::string KPatientStringOperation::AssembleDicomPatientName(const std::string &a,
                                                              const std::string &b,
                                                              const std::string &c)
{
    // Склейка через '^' с обрезкой хвостовых пустых компонент (точная форма реф.
    // не полностью восстановлена — принят разумный вариант).
    std::string out = a;
    if (!b.empty() || !c.empty())
        out += "^" + b;
    if (!c.empty())
        out += "^" + c;
    return out;
}

std::string KPatientStringOperation::AssembleDicomFilePath(const std::string &dir,
                                                           const std::string &name, bool flag)
{
    std::string out = dir + "/" + name;
    if (!flag)
        out += ".dcm";   // flag==true → без расширения
    return out;
}

std::string KPatientStringOperation::AssembleCoverFilePath(const std::string &dir,
                                                           const std::string &name)
{
    const std::string base = dir + "/" + name;
    for (const char *ext : {".jpeg", ".bmp", ".png"}) {
        const std::string path = base + ext;
        if (QFile::exists(QString::fromStdString(path)))
            return path;
    }
    return std::string();   // ни одного файла — реф. возвращает пусто
}

std::string KPatientStringOperation::GenerateUniqueIdentifier(const std::string &type)
{
    // Корень UID по сущности (реф. s_mapTypeToUIDRoot → суффикс .1.x).
    struct Row { const char *type; const char *suffix; };
    static const Row table[] = {
        {"Patient", ".1.1"}, {"DcmStudy", ".1.2"}, {"DcmSeries", ".1.3"},
        {"Report", ".1.4"}, {"CodeSequence", ".1.5"}, {"Worklist", ".1.6"},
        {"PerformedProcedureStep", ".1.7"}, {"Transaction", ".1.8"},
        {"DicomCommand", ".1.9"}, {"Querylist", ".1.10"},
    };
    for (const Row &r : table) {
        if (type == r.type) {
            // Реф. далее зовёт DCMTK dcmGenerateUniqueIdentifier с этим корнем; базовый
            // UID-root и генерируемый хвост — device/DCMTK, здесь не воспроизводимы.
            return std::string(r.suffix);
        }
    }
    return std::string();
}
