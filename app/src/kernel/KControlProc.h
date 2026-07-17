#pragma once

#include <QString>

class yxyDES2;

// Тип лицензии машинного контроля (реф. _KControlType). Значения выведены из switch'ей
// KControlProc::LicenseFileName/IsMd5sumUsed (исчерпывающе 0..5); строки — имена секций/ключей ini.
enum _KControlType {
    KCT_PROCESSOR_RELEASE = 0,   // "processor_release" — процессор, постоянная лицензия
    KCT_PROCESSOR_DELAY   = 1,   // "processor_delay"   — процессор, временная
    KCT_IMPORT_ENDO       = 2,   // "import_endo"       — импорт лицензии эндоскопа
    KCT_ENDO_RELEASE      = 3,   // "endo_release"      — эндоскоп, постоянная
    KCT_ENDO_DELAY        = 4,   // "endo_delay"        — эндоскоп, временная
    KCT_IMPORT_PROCESSOR  = 5,   // "import_processor"  — импорт лицензии процессора
};

// Процедуры машинного контроля/лицензирования (реф. KControlProc, X-2600). STANDALONE и
// STATELESS: без vtable/typeinfo/Q_OBJECT, ctor/dtor пустые, полей нет (методы инстансные —
// сохраняем 1:1, как у KMeaStringUtil). Крипто — поверх [[yxyDES2]] (проверенный DES).
//
// РЕАЛИЗОВАНО ЗДЕСЬ (сверено дизасмом): крипто-ядро + чистые хелперы. Остальные ~25 методов
// класса (Start/StopEndoMc, IsEndoMatch, CheckLicense, GetMcFilenameList, ImportMatchLicense…)
// — отдельный заход (часть device/ini-bound).
class KControlProc
{
public:
    KControlProc() = default;
    ~KControlProc() = default;

    // ASCII-hex → байты шифртекста. Возврат — число байт, либо -1 при nullptr-аргументе.
    // nBits = ((strlen(inHex)+3)/4)*16 (длина ОКРУГЛЯЕТСЯ ВВЕРХ до кратной 4 hex-символам);
    // out должен вмещать nBits/8 байт. Hex2Bits портит вход → работает по копии (реф. 16КБ буфер).
    int ConvertOtherFormat2Ciphertext(yxyDES2 *des, char *out, char *inHex);

    // Hex-шифртекст → Latin-1 открытый текст. Ключ DES — литерал **"ZXYuio12"**, слот 0
    // (реф. собирает ключ инлайн: 0x32316F6975595 85A → LE-байты 5A 58 59 75 69 6F 31 32).
    // Плейнтекст обрезается по первому NUL (реф. strlen).
    QString DecryptionStr(QString &cipherHex);

    // Файловая обёртка (реф. внутреннее имя ControlInput::CipherFile2PlainFile): читает файл
    // шифртекста целиком → DecryptionStr → пишет открытый текст в KControlINI::PlainINIpath().
    // ВОЗВРАЩАЕТ ПУТЬ записанного файла (НЕ сам плейнтекст); "" при любой ошибке открытия.
    QString Cipher2Plain(QString &cipherFilePath);

    // Имя ini-файла лицензии: sn + суффикс по типу (0,3→"_release.ini"; 1,4→"_delay.ini";
    // 2,5→"_import.ini"; >=6 → пустая строка).
    QString LicenseFileName(QString sn, _KControlType type);

    // Несмотря на имя — НЕ конвертер строк: считает и СОХРАНЯЕТ дедлайн через KControlINI.
    // no-op, если контроль времени выключен либо остаток дней <= 0. Формат — "yyyy-MM-dd"
    // (и разбор, и вывод). Непарсимая дата → невалидный QDate → в SetDeadline уйдёт пустая строка.
    void SystemDate2MCDate(QString date);
};
