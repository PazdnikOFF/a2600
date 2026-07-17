#pragma once

#include "kernel/KControlINI.h"   // _MC_Time / _MC_Endo + статический API ini

#include <QString>
#include <QStringList>

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

    // --- лицензирование (ini/файлы поверх DES) ---
    // Каталог лицензий: реф. KSystem::ImportPath() + "license/" (ImportPath — USB-корень,
    // DEVICE). Off-device: settable override (SetImportRoot) + "license/".
    QString McDirLicense() const;
    bool    IsLicensePathExist() const;           // QDir(McDirLicense()).exists()
    QStringList GetMcFilenameList() const;        // *.ini в каталоге лицензий (реф. USB-gate опущен)

    // Анти-повтор md5 (реф. IsMd5sumUsed): по ИМЕНИ ТИПА как ключу в licensehistory.ini хранится
    // QStringList уже израсходованных md5. Уже есть → true; иначе ДОБАВЛЯЕТ и пишет (side-effect) →
    // false. type>=6 → true без обращения к файлу. Аргумент — сам md5 (не серийник, sic).
    bool IsMd5sumUsed(QString md5, _KControlType type);

    // Проверка лицензии (реф. CheckLicense → int-код): 1 нет каталога; 2 нет файла; 3 расшифровка
    // не удалась; 4 md5 использован ИЛИ поля не совпали; 0 OK. Файл LicenseFileName(sn,type) в
    // каталоге → Cipher2Plain → plain.ini: md5sum (анти-повтор) + SN/ControlState/ProcessorSN/
    // EndoSN по типу (см. .cpp). ProcessorSN машины — KSystemSet::GetProcessorSN().
    int CheckLicense(QString sn, _KControlType type);

    // Читают plain.ini (KControlINI::PlainINIpath()) — ОТЛИЧНЫ от одноимённых KControlINI (control.ini):
    QString GetDeadline() const;    // ключ "deadline", дефолт ""
    int     GetDelayTime() const;   // ключ "addNum", дефолт 0

    bool IsStartTimeMc() const;     // → KControlINI::IsStartTimeControl()
    bool IsStartEndoMc() const;     // → KControlINI::IsStartEndoControl()
    QString GetCurEndoSN() const;   // DEVICE (живой эндоскоп); off-device — settable (SetCurEndoSN)

    // Заблокирована ли машина (реф. IsOutofControl): истёк лимит времени (IsStartTimeMc && remain==0)
    // ЛИБО включён endo-контроль и текущий эндоскоп НЕ в списке разрешённых.
    bool IsOutofControl();

    // --- контроль времени (control.ini + арифметика дат) ---
    // Лог + СКВОЗНАЯ запись KControlINI::WriteMcTime(t). ВНИМАНИЕ: сам НЕ ставит controlTime —
    // поля (включая флаг и remainDays) целиком на совести вызывающего. nullptr → тихий no-op.
    void StartTimeMc(const _MC_Time *t);
    // Снятие контроля: controlTime=false, deadline="2099-01-01", **remainDays = -1** (sic —
    // не 0: IsOutofControl проверяет ==0, поэтому -1 это «разоружено», а не «истекло»).
    void StopTimeMc();
    // Ежедневный пересчёт остатка — МОНОТОННЫЙ ХРАПОВИК: remain = min(remain, max(0, daysTo(deadline))).
    // Только уменьшается: перевод часов НАЗАД не возвращает дни (запись просто не делается),
    // перевод ВПЕРЁД сжигает их безвозвратно. Непарсимый deadline → remain=0 (fail-closed).
    // no-op при выключенном контроле либо remain<=0. ЧИТАЕТ СИСТЕМНЫЕ ЧАСЫ (QDate::currentDate).
    void UpdateMcDays();

    // --- контроль эндоскопов (control.ini, блочная запись) ---
    // controlEndo=true + список → ОДНА запись WriteMcEndo (флаг и список не могут разойтись).
    // Реф. лога НЕ пишет (асимметрия со StopEndoMc — так в оригинале).
    void StartEndoMc(QStringList endos);
    void StopEndoMc();                              // лог + controlEndo=false, список пуст
    // Текущий эндоскоп в списке разрешённых? Сравнение Qt::CaseSensitive (явно в реф.).
    bool IsEndoMatch();

    // Off-device-параметризация DEVICE-зависимостей (реф. USB/эндоскоп).
    static void SetImportRoot(const QString &dir);   // корень импорта (реф. KSystem::ImportPath)
    static void SetCurEndoSN(const QString &sn);     // текущий серийник эндоскопа

    // НЕ РЕАЛИЗОВАНО — DEVICE-BOUND (реф. GetEndoScope()->GetEepromData()/IsEndoReady(),
    // KEndoScope в app/src отсутствует; EEPROM живого эндоскопа):
    //   GetEndoRemainTimes()  — EEPROM +0x24 (u16)
    //   GetEndoDeadline()     — EEPROM +0x28 (QString)
    //   GetMatchProcessorList() — EEPROM +0x30 (QStringList) — NB: имя врёт, matchprolist.ini НЕ читает
    //   IsStartMatchProcessorCtrl() — KEndoScope::IsOpenMatchProControl()
    //   IsStartEndoUseTimeCtrl()    — IsEndoPowerOn() && KEndoScope::IsOpenEndoControl() (бит 6 флагов)
    //   IsEndoPowerOn()             — KEndoScope::IsEndoReady() (поле состояния +0x10 == 4)
};
