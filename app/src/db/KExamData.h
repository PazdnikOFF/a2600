#pragma once

#include <functional>
#include <string>
#include <vector>

// Утилиты имён/типов файлов данных осмотра (реф. KExamData, X-2600 — набор
// СТАТИЧЕСКИХ методов, экземпляра нет). Пара к [[KExamDataFileNameGenerator]]:
// генератор строит имя, а KExamData описывает формат и решает, когда нумерацию
// пора остановить.
//
// ЧТО ЗДЕСЬ ЧИСТО OFF-DEVICE и портировано 1:1 по дизасму:
//   • GetFileFormat — отображение типа файла в расширение;
//   • композиция IsDoNotGenerateFileName = IsHaveMaxSerialNum || total > 998
//     (порог 998 = `cmp #0x3e6` в реф. @0x48dbbc);
//   • разбор examId из пути (split по '/').
// ЧТО ОПИРАЕТСЯ НА ФС (реф. GetAllFiles enumerates каталог осмотра): у нас за
// инъектируемым провайдером — off-device решать нечего, но композиция и пороги
// сохранены точно.
namespace KExamData {

// Реф. E_FILE_TYPE. GetFileFormat @0x48b7d0: 0 → ".jpg", 1 → ".mp4", иначе "".
enum E_FILE_TYPE { FT_IMAGE = 0, FT_VIDEO = 1 };
std::string GetFileFormat(int type);

// Реф. GetExamIdFromFileName @0x48bc08: если в пути есть '/', отрезает всё до
// последнего '/' (берёт basename), затем ограничивает результат первыми 13
// символами (реф. `mov x0,#0xd`). Возврат: 0 — успех, иначе непусто/ошибка.
// Здесь чистая строковая логика.
int GetExamIdFromFileName(const std::string &fullPath, std::string &examIdOut);

// --- Ниже в реф. — перечисление файлов каталога осмотра (GetAllFiles, ФС). ---
// У нас за инъектируемым провайдером: {есть ли файл с максимальным серийником
// «999^…», сколько всего файлов с валидным серийником}. Дефолт — {false, 0}.
struct SerialInfo { bool haveMax = false; int total = 0; };
void SetSerialInfoProvider(std::function<SerialInfo(const std::string &)> f);

bool IsHaveMaxSerialNum(const std::string &examId);      // реф. @0x48d5e0
int  GetTotalFileSerialNumber(const std::string &examId);// реф. @0x48d738

// Реф. @0x48db98: IsHaveMaxSerialNum(id) || GetTotalFileSerialNumber(id) > 998.
bool IsDoNotGenerateFileName(const std::string &examId);

} // namespace KExamData
