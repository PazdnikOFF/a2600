#pragma once

#include <QString>
#include <QVector>

#include <string>

// Парсер скриптов автотеста (реф. — процесс **X2000Simulator**, НЕ X2000!).
// ВАЖНАЯ НАХОДКА РЕВЕРСА: литерала "Confiure" в X2000 НЕТ ВООБЩЕ. Разбор `[Confiure]`-
// скриптов живёт в отдельном бинарнике update/root/X2000Simulator (aarch64 PIE, не стрипнут);
// в X2000 находится только ПРИЁМНАЯ половина — KAutoTestThread (получает уже готовые коды по
// IPC и инжектит их в uinput). Поэтому реф.-имена здесь — СВОБОДНЫЕ C-ФУНКЦИИ симулятора
// (INIFileCaseExec/AnalyseLineCase/ResetKeyValue/GetCaseValue/OneKeyExec/SendOneKey), а не
// методы класса.
//
// Разбор — РУЧНОЙ C, НЕ KConfig и НЕ QSettings: fgets(512) → strncasecmp по префиксу →
// strstr("=") → atoi. Следствия (воспроизводим 1:1): комментарии '#'/';' НЕ поддерживаются,
// кавычек нет, табы не срезаются, неизвестные строки молча игнорируются.
//
// РЕАЛИЗОВАНО OFF-DEVICE: разбор + модель скрипта + таблица кодов + рекурсия Script.
// НЕ РЕАЛИЗОВАНО (device): SendOneKey (IPC KMsgBuf→приложение), Simulate_key/mouse
// (/dev/uinput), а также вся приёмная сторона X2000 (KAutoTestThread::run/
// KeyboardSimulation/PanelKeySimulation).

// Один шаг скрипта (реф. KEY_CONFIG). Смещения полей — как в симуляторе.
struct KEY_CONFIG {
    int  code  = -1;   // +0x00 — токен из stKeyValueStr; -1 = не распознан (OneKeyExec → false)
    int  event = 0;    // +0x04 — уходит в старшие 16 бит param
    int  ctrl  = 0;    // +0x08 — ненулевой → OR 0x40000000
    int  alt   = 0;    // +0x0c — ненулевой → OR 0x80000000
    int  shift = 0;    // +0x10 — ненулевой → OR 0x20000000
    int  value = 0;    // +0x14 — младшие 16 бит param; для code=Sleep — длительность (мс!)
    int  sleep = 0;    // +0x18 — пауза ПОСЛЕ нажатия (мс), внутри SendOneKey
    int  cnt   = 1;    // +0x1c — счётчик повторов ТОЛЬКО для Script/Cmd (дефолт 1)
    std::string cmd;   // +0x20 — char[256]; под-скрипт или shell-команда
};

// Заголовок скрипта (реф. INI_CONFIG).
struct INI_CONFIG {
    int num  = 0;   // +0x00 — ЧИТАЕТСЯ, НО НИГДЕ НЕ ИСПОЛЬЗУЕТСЯ (итерацию не ограничивает!)
    int time = 0;   // +0x04 — пауза между шагами (мс), usleep(time*1000) после каждого
};

// Псевдо-опкоды (реф. значения в stKeyValueStr). В таблице реф. поле кода — uint32,
// а в KEY_CONFIG оно знаковое (дефолт -1), поэтому храним ТОТ ЖЕ 32-битный образ в int.
constexpr int AUTOTEST_CODE_SLEEP  = static_cast<int>(0xffff0002u);
constexpr int AUTOTEST_CODE_CMD    = static_cast<int>(0xffff0003u);
constexpr int AUTOTEST_CODE_SCRIPT = static_cast<int>(0xffff0004u);

// Коды возврата AnalyseLineCase (реф. 1:1).
enum {
    LINE_UNKNOWN   = 0,   // строка не распознана — молча игнорируется
    LINE_CONFIURE  = 1,   // "[Confiure]" (опечатка оригинала)
    LINE_ENV       = 2,   // env=… — значение ОТБРАСЫВАЕТСЯ
    LINE_NUM       = 3,
    LINE_TIME      = 4,
    LINE_KEY       = 5,   // "[Key…" — СБРАСЫВАЕТ (flush) предыдущий накопленный шаг
    LINE_FIELD     = 6,   // code/event/ctrl/alt/shift/value/sleep/cnt/cmd (реф. 6..14)
};

class KAutoTestScript
{
public:
    // Реф. ResetKeyValue(KEY_CONFIG*) — code=-1, целые 0, cnt=1, cmd очищается.
    static void ResetKeyValue(KEY_CONFIG &key);

    // Реф. GetCaseValue(char*) — strstr(line,"=")+1, затем пропуск пробелов.
    // КВИРК: если '=' НЕТ — реф. возвращает NULL, и вызывающий делает strlen(NULL) → падение.
    // У нас: found=false (падение не воспроизводим — это дефект, а не поведение).
    static std::string GetCaseValue(const std::string &line, bool *found = nullptr);

    // Реф. AnalyseLineCase(char*, KEY_CONFIG*, INI_CONFIG*) → код строки (см. enum выше).
    // Сопоставление префиксов — БЕЗ УЧЁТА РЕГИСТРА (strncasecmp), с начала строки.
    static int AnalyseLineCase(const std::string &line, KEY_CONFIG &key, INI_CONFIG &ini);

    // Имя токена → код (таблица реф. stKeyValueStr @0x20318, 146 записей).
    // Не найдено → -1. Сопоставление ТОЧНОЕ (регистр учитывается).
    static int KeyValueFromName(const std::string &name);
    static int KeyValueTableSize();

    // Реф. INIFileCaseExec(FILE*, const char*) — потоковый разбор.
    // ОТЛИЧИЕ ОТ РЕФ.: оригинал ВЫПОЛНЯЕТ каждый шаг (OneKeyExec→SendOneKey→IPC/uinput);
    // off-device мы СОБИРАЕМ последовательность шагов в outKeys (исполнение — device).
    // Порядок шагов = ПОРЯДОК В ФАЙЛЕ (не Key0..Key<num-1>, не сортировка): любой заголовок
    // с префиксом "[Key" (включая "[KeyEnd]") сбрасывает предыдущий шаг; номер НЕ разбирается.
    // Рекурсия: code=Script → под-скрипт AutotestDir()+cmd, повторяется cnt раз.
    // ВНИМАНИЕ (квирк реф.): ОГРАНИЧЕНИЯ ГЛУБИНЫ И ЗАЩИТЫ ОТ ЦИКЛОВ НЕТ — самоссылающийся
    // cmd рекурсирует до исчерпания стека. Сохраняем 1:1; циклы в скриптах недопустимы.
    static bool INIFileCaseExec(const QString &file, QVector<KEY_CONFIG> &outKeys,
                                INI_CONFIG &outIni);

    // Реф. литерал "/home/root/data/app/autotest/" (= AppPath() + "autotest/").
    static QString AutotestDir();
};
