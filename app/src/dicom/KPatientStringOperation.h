#pragma once

#include <string>

// Строковые/DICOM-утилиты для данных пациента (реф. KPatientStringOperation, X-2600).
// НЕ UI, НЕ device: чистые std::string-функции, все static, состояние — только
// function-local static-таблицы. Потребители — DICOM-слой (KDcmTaskWorklist,
// KDicomInterface, convertDcmDataCharset).
//
// StringReplace/StringTrim в оригинале дублируют KMeaStringUtil::ReplaceStr/Trim —
// сохранены под именами оригинала (реф. держит их отдельно).
class KPatientStringOperation
{
public:
    static void StringReplace(std::string &s, const std::string &from, const std::string &to);
    static void StringTrim(std::string &s);   // trim по фикс. набору " \r\n\t"
    // Заменяет каждый символ из набора \ / : * ? " < > | на строку repl.
    static void ReplaceInvalidCharInFolderName(std::string &s, const std::string &repl);

    // DICOM SOP Class UID по типу снимка (реф. таблица):
    // type 0 → flag? US Multiframe : VL Endoscopic; 1 → Secondary Capture; 2 → US Image; иначе "".
    static std::string GetSOPClassUID(int type, bool flag);

    // iconv-конвертация io на месте (from→to), буфер 5120 байт (усечение сверх).
    // "" → true (no-op); iconv_open fail → false.
    static bool ConvertCharacterset(const std::string &from, const std::string &to,
                                    std::string &io);
    // Пробует GB2312/ISO-8859-1/ISO-8859-5 → utf-8, true при первом успехе.
    static bool ConvertCharactersetToUTF8(std::string &io);

    // DICOM defined-term (ISO_IR 100/144/192/GB18030…) → имя iconv-charset.
    // out — флаг распознавания (точная семантика в реф. не подтверждена — см. .cpp).
    static std::string GetISOCharactersetOfDicom(const std::string &term, bool &out);

    // DICOM Person Name split по '^'. Порядок реф. (буквально): token0→p4, token1→p2,
    // token2→p3; без '^' → p2 = вся строка.
    static void SplitDicomPatientName(const std::string &src, std::string &p2,
                                      std::string &p3, std::string &p4);
    // Обратная склейка через '^' (обрезка хвостовых '^' — реф. не полностью определена).
    static std::string AssembleDicomPatientName(const std::string &a, const std::string &b,
                                                const std::string &c);

    // dir + "/" + name; при flag==false дополнительно + ".dcm".
    static std::string AssembleDicomFilePath(const std::string &dir, const std::string &name,
                                             bool flag);
    // dir + "/" + name + первое существующее из {.jpeg,.bmp,.png} (QFile::exists).
    static std::string AssembleCoverFilePath(const std::string &dir, const std::string &name);

    // Корень UID по типу сущности (Patient/DcmStudy/… → суффикс .1.1 … .1.10) +
    // финальный DCMTK dcmGenerateUniqueIdentifier. Полная генерация — device/DCMTK:
    // здесь возвращается только root+суффикс (см. .cpp).
    static std::string GenerateUniqueIdentifier(const std::string &type);
};
