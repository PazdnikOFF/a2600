#pragma once

#include <QString>
#include <QStringList>

#include <string>
#include <vector>

#include "report/KScopeClass.h"

// Источник данных редактора отчёта (реф. KReportEditDataSource, X-2600;
// исходник реф. — dialog/patient/reportedit/KReportEditDataSource.cpp).
//
// РЕВЕРС: класс СТАТЕЛЕСС — ctor голый `ret`, ни vtable, ни typeinfo, ни полей,
// ни синглтона; ВСЕ 17 методов СТАТИЧЕСКИЕ. Реализовано так же.
//
// ⚠️ КОЛЛИЗИЯ ИМЁН: наш `report/KReportDataSource.h` в комментарии ссылается на
// «реф. KReportEditDataSource», но это СОВСЕМ ДРУГОЙ класс (мешок ключ-значение
// QHash<QString,QString> с SetValue/GetValue/HasValue). Общих методов — НОЛЬ.
// Настоящий KReportEditDataSource — здесь; путать их нельзя.
class KReportEditDataSource
{
public:
    // --- сериализация списка файлов (точно обратимая пара) ------------------
    // Реф.: пустой список → ""; иначе "$" + для каждого элемента "#"+элемент.
    // Например {"a.jpg","b.jpg"} → "$#a.jpg#b.jpg".
    // ЭКРАНИРОВАНИЯ НЕТ — '#' внутри имени файла ломает обратимость.
    static QString ChangeFileListToString(QStringList list);
    // Реф.: пусто, если НЕ (left(1) == "$" И length() > 2). Иначе
    // mid(2, length()).split("#", KeepEmptyParts) — то есть отбрасывается "$#",
    // ведущая пустая часть не появляется, а хвостовой '#' даёт пустой элемент.
    static QStringList ChangeStringToFileList(QString str);

    // --- списки названий органов --------------------------------------------
    // Реф.: НИКАКИХ файлов/БД — зашитые списки QObject::tr(ключ).
    // Цепочка сравнений только на 1/2/3; всё остальное (включая 0 и 14) —
    // гастро-список.
    static QStringList GetSysOrganNameList(KScopeClass::E_CLASS cls);
    // Реф.: <usrDir>/patient/posname/<DEV_*>.xml, формат — boost::serialization
    // XML, NVP-тег `obj_organ_name_list`. Если файла нет/не читается или список
    // пуст → ВОЗВРАЩАЕТ GetSysOrganNameList(cls).
    // ВАЖНО: слияния/сортировки/дедупликации НЕТ — списки альтернативны.
    static QStringList GetUserOrganNameList(KScopeClass::E_CLASS cls);
    // Реф.: void (кода возврата нет). ПОЛНАЯ ПЕРЕЗАПИСЬ файла, без чтения-слияния;
    // при неудаче открытия создаёт каталог (mkpath) и повторяет попытку.
    static void SetUserOrganNameList(KScopeClass::E_CLASS cls, const QStringList &list);

    // --- пути к изображениям ------------------------------------------------
    // Реф.: GetReadOnlyBaseDir() + "mainapp/patient/region/<Орган>/1.jpg";
    // для неизвестного класса — "" (а НЕ базовый каталог).
    static std::string GetRegionImgPath(KScopeClass::E_CLASS cls);
    // Реф.: GetReadOnlyBaseDir() + "mainapp/application/qss/" + <icon/arrow/*.png>;
    // для 0/неизвестного относительная часть пуста ⇒ возвращается ГОЛЫЙ базовый
    // каталог (в отличие от GetRegionImgPath).
    static std::string GetCursorImgPath(exam_detail::E_CURSOR_TYPE type);

    // --- имена ---------------------------------------------------------------
    // Реф.: TR_VGReport/TR_VCReport/TR_VBReport/TR_VNReport/TR_VDReport;
    // для неизвестного класса — пустая строка.
    static QString GetReportTypeName(KScopeClass::E_CLASS cls);
    // Реф.: `mov w0,#0; ret` — константа 0. Вызовов в бинарнике НЕТ (мёртвый
    // символ); тип возврата НЕ УСТАНОВЛЕН, известно лишь «32-битный 0».
    static int GetSysDeviceType();

    // --- помощники чтения из БД ---------------------------------------------
    // Реф.: сравнивает с файл-статическим "INVALID_STRING"; равно → "", иначе
    // сама строка.
    static std::string LoadDbString(const std::string &s);
    // Реф.: std::stoi(base 10). Успех → пишет out и возвращает 0.
    // Ошибки: -2 / -3 / -1 (соответствие invalid_argument/out_of_range/прочее
    // НЕ УСТАНОВЛЕНО точно); out при ошибке НЕ ТРОГАЕТСЯ.
    static int LoadDbInt(const std::string &s, int &out);

    // --- таблица отчётов ------------------------------------------------------
    static int DeleteReportByExamId(const QString &examId);
    // Реф.: проба существования — заводит временную запись, зовёт GetEntity,
    // запись выбрасывает, код возврата отдаёт как есть.
    static int QueryReportTable(const QString &examId);
    // Реф.: 1 — успех, -1 — ошибка. Список снимков берётся из колонки ExamImg
    // через ChangeStringToFileList и ФИЛЬТРУЕТСЯ по фактическому наличию файла.
    static int GetOneRecordFromReportTB(const std::string &examId,
                                        std::vector<std::string> &outImgs,
                                        std::string &outPath);
};
