#include "db/KExamData.h"

namespace KExamData {

namespace {
std::function<SerialInfo(const std::string &)> g_serialProvider;
}

std::string GetFileFormat(int type)
{
    // Реф. @0x48b7d0 — switch ровно по двум значениям, всё прочее → "".
    switch (type) {
    case FT_IMAGE: return ".jpg";
    case FT_VIDEO: return ".mp4";
    default:       return std::string();
    }
}

int GetExamIdFromFileName(const std::string &fullPath, std::string &examIdOut)
{
    // Реф. @0x48bc08: работает только если строка непуста и содержит '/'.
    if (fullPath.empty())
        return 0;                       // реф.: пустой вход → ранний выход, examId не тронут
    const std::size_t slash = fullPath.find('/');
    if (slash == std::string::npos)
        return 0;                       // нет разделителя → ошибка-ветка реф. (examId пуст)

    // Реф.: находит ПОСЛЕДНИЙ '/', берёт basename (substr после него); при
    // отсутствии — первые 13 символов. У нас: basename после последнего '/',
    // ограниченный 13 символами (реф. `mov x0,#0xd`).
    const std::size_t last = fullPath.find_last_of('/');
    const std::string base = fullPath.substr(last + 1);
    examIdOut = base.substr(0, 13);
    return 0;
}

void SetSerialInfoProvider(std::function<SerialInfo(const std::string &)> f)
{
    g_serialProvider = f;
}

bool IsHaveMaxSerialNum(const std::string &examId)
{
    // Реф. @0x48d5e0: перечисляет файлы каталога и ищет отметку «999^…»
    // (максимальный серийник). У нас — из провайдера.
    return g_serialProvider ? g_serialProvider(examId).haveMax : false;
}

int GetTotalFileSerialNumber(const std::string &examId)
{
    // Реф. @0x48d738: считает файлы с валидным серийником в каталоге осмотра.
    return g_serialProvider ? g_serialProvider(examId).total : 0;
}

bool IsDoNotGenerateFileName(const std::string &examId)
{
    // Реф. @0x48db98: IsHaveMaxSerialNum(id) ЛИБО total > 998 (порог `cmp #0x3e6`).
    if (IsHaveMaxSerialNum(examId))
        return true;
    return GetTotalFileSerialNumber(examId) > 998;
}

} // namespace KExamData
