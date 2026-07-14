#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

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

// Метаданные шаблона (реф. KTempletBaseInfo).
struct KTempletBaseInfo {
    QString name;                    // атрибут name (="NP-2x2")
    QString modifyDate;              // modifydate ("factory" для заводских)
    bool    factory = false;        // factory="1" — заводской шаблон
    QVector<KTempletDept> depts;     // список департаментов

    QStringList deptNames() const;   // имена департаментов
    bool hasDept(const QString &dept) const;
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

private:
    KSysReportTempletCfg() = default;
    bool loaded_ = false;
    QString reportRoot_;
    QVector<KTempletBaseInfo> infos_;
    QVector<KTempletBaseInfo> libInfos_;
    void ensureLoaded() const;
    // Разбор одного каталог-файла (config/<file>) в vector<KTempletBaseInfo>.
    bool parseTempletFile(const QString &file, QVector<KTempletBaseInfo> &out) const;
};
