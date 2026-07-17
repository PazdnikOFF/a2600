#pragma once

#include "report/KReportTemplateData.h"   // KReportTemplateDataNew

#include <QString>
#include <QStringList>
#include <QVector>

#include <map>
#include <string>
#include <vector>

// Каталог шаблонов отчёта с департаментами (реф. KSysReportTempletCfg / KTempletBaseInfo,
// X-2600). Читает config/TempletInfo.xml:
//   <Templet name="NP-2x2" modifydate="factory" factory="1">
//     <Dept name="KW_NP-2x2" default="1" />
//   </Templet>
// Даёт каталог шаблон↔департамент поверх KReportTemplateManager (реф. LoadTempletInfo,
// GetDeptsOfTemplate, GetTempletNamesByDept, GetTemplateInfoByName, LoadDefault).

// Департамент шаблона (реф. Dept: name + default-флаг).
struct KTempletDept {
    QString name;              // напр. "KW_NP-2x2"
    bool    isDefault = false; // атрибут default="1"
};

// Метаданные шаблона (реф. KTempletBaseInfo, sizeof 0x78).
// ВНИМАНИЕ (сверено дизасмом): в реф. департаменты — это `std::map<std::string,bool>`
// (УПОРЯДОЧЕННЫЙ, УНИКАЛЬНЫЙ по имени: AddDept на существующий ПЕРЕЗАПИСЫВАЕТ флаг, обход —
// по возрастанию имени). У нас QVector<KTempletDept> — мутаторы ниже ВОСПРОИЗВОДЯТ map-семантику
// (уникальность + сортировка по имени), чтобы поведение совпадало с оригиналом.
struct KTempletBaseInfo {
    QString name;                    // атрибут name (="NP-2x2")
    QString modifyDate;              // modifydate ("factory" для заводских)
    bool    factory = false;        // factory="1" — заводской шаблон
    QVector<KTempletDept> depts;     // список департаментов (держим отсортированным по name)

    QStringList deptNames() const;   // имена департаментов
    bool hasDept(const QString &dept) const;

    // --- мутаторы/аксессоры реф. (нужны KSysReportTempletModel/Control) ---
    // Имена как в реф. (TempletName/SetTempletName/…); наши поля публичны, но модель
    // работает через эти методы — сохраняем совместимость 1:1 по семантике.
    QString TempletName() const { return name; }
    void    SetTempletName(const QString &v) { name = v; }
    QString ModifyDate() const { return modifyDate; }
    void    SetModifyDate(const QString &v) { modifyDate = v; }
    bool    IsFactory() const { return factory; }
    void    SetFactory(bool v) { factory = v; }

    bool HasDept(const QString &dept) const { return hasDept(dept); }
    bool IsDefault(const QString &dept) const;          // флаг default департамента (нет → false)
    void SetDefault(const QString &dept, bool v);       // только если департамент есть
    void AddDept(const QString &dept, bool isDefault);  // map-семантика: есть → перезапись флага
    void DeleteDept(const QString &dept);
    QStringList GetDepts() const { return deptNames(); }
};

class KSysReportTempletCfg
{
public:
    static KSysReportTempletCfg &GetInstance();

    // Корень report/ (по умолчанию — как у KReportTemplateManager).
    void SetReportRoot(const QString &dir) { reportRoot_ = dir; }
    QString ReportRoot() const;

    // Перечитать оба каталога (реф. LoadTempletInfo + LoadTempletLibInfo). true при успехе
    // разбора config/TempletInfo.xml (основной каталог).
    bool Reload();

    // Все шаблоны каталога (реф. GetTempletInfos) — config/TempletInfo.xml,
    // короткие имена ("NP-2x2").
    const QVector<KTempletBaseInfo> &TempletInfos() const { return infos_; }
    QStringList TempletNames() const;

    // Библиотека шаблонов (реф. GetTempletLibInfos) — config/TempletLibInfo.xml,
    // полные имена layout-файлов ("ReportTemplateNP-2x2"). Та же схема XML.
    const QVector<KTempletBaseInfo> &TempletLibInfos() const { return libInfos_; }
    QStringList TempletLibNames() const;
    KTempletBaseInfo GetLibTemplateInfoByName(const QString &name, bool *found = nullptr) const;

    // Инфо по имени (реф. GetTemplateInfoByName). found=false → пустая структура.
    KTempletBaseInfo GetTemplateInfoByName(const QString &name, bool *found = nullptr) const;

    // Департаменты шаблона (реф. GetDeptsOfTemplate).
    QStringList GetDeptsOfTemplate(const QString &name) const;
    // Шаблоны, относящиеся к департаменту (реф. GetTempletNamesByDept).
    QStringList GetTempletNamesByDept(const QString &dept) const;
    // Дефолтный шаблон для департамента (первый Dept с default="1").
    QString GetDefaultTempletByDept(const QString &dept) const;

    // --- реф.-методы записи/загрузки (нужны KSysReportTempletModel) ---
    // ВАЖНО: реф. НЕ использует один reportRoot_ — он жёстко ходит в KEnvConfig, и RO/RW это
    // РАЗНЫЕ ПРЕФИКСЫ (не суффикс одного корня), сверено дизасмом:
    //   RO = GetReadOnlyBaseDir() + "mainapp/patient/report/config/TempletInfo.xml"
    //   RW = GetUsrDir()          + "patient/report/config/TempletInfo.xml"
    // Эти пять методов ходят по KEnvConfig (как реф.); старый read-путь на reportRoot_ сохранён.

    // Сброс к заводским (реф. LoadDefault): удаляет user-cfg КАЖДОГО шаблона через
    // KTemplateCfg::DeleteTemplateCfg, сбрасывает либ-кэш (UpdateTemplateLib(пустой)), затем
    // читает RO-каталог в infos_. НА ДИСК TempletInfo.xml НЕ ПИШЕТ (user-файл остаётся старым).
    void LoadDefault();
    // Перечитать USER-каталог (реф. LoadUserXML — путь DiscardChange). Без CheckFile/RO-фолбэка.
    void LoadUserXML();
    // Полный cfg шаблона по имени (реф. — делегирует KTemplateCfg::GetTemplateCfg: файл
    // template/FullTemplate/Template(<имя>).xml, user→RO-фолбэк + кэш).
    void GetTemplateByName(const std::string &name, KReportTemplateDataNew &out);
    // Пишет USER TempletInfo.xml (реф. пишет ВСЕ инфо, без фильтра; RO не трогает).
    void SaveTempletInfos(const QVector<KTempletBaseInfo> &infos);
    // Реф. порядок: СНАЧАЛА удаления (delList → KTemplateCfg::DeleteTemplateCfg), затем апдейты.
    // ФИЛЬТР: запись пропускается для НЕТРОНУТЫХ заводских (factory==true && modifydate=="factory").
    void SaveTemplateCfg(const std::map<std::string, KReportTemplateDataNew> &cfgs,
                         const std::vector<std::string> &delList);

private:
    // Пути реф. (KEnvConfig), см. комментарий выше.
    static QString roTempletInfoFile();
    static QString rwTempletInfoFile();
    // Запись каталога в XML (реф. KTempletInfoFileHandler::Save + AddTempletNode, pugixml):
    // <Root><Templet name= modifydate= factory="1|0"><Dept name= default="1|0"/></Templet></Root>
    // Bool — РОВНО "1"/"0"; порядок атрибутов name,modifydate,factory / name,default;
    // <Dept> — ПО ВОЗРАСТАНИЮ имени (в реф. depts это std::map).
    static bool saveTempletFile(const QString &file, const QVector<KTempletBaseInfo> &infos);
    KSysReportTempletCfg() = default;
    bool loaded_ = false;
    QString reportRoot_;
    QVector<KTempletBaseInfo> infos_;
    QVector<KTempletBaseInfo> libInfos_;
    void ensureLoaded() const;
    // Разбор одного каталог-файла (config/<file>) в vector<KTempletBaseInfo>.
    bool parseTempletFile(const QString &file, QVector<KTempletBaseInfo> &out) const;
    // Тот же разбор, но по АБСОЛЮТНОМУ пути (реф.-методы через KEnvConfig RO/RW).
    bool parseTempletFileAbs(const QString &absFile, QVector<KTempletBaseInfo> &out) const;
};
