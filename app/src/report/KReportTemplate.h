#pragma once

#include <QString>
#include <QVector>

// Дерево элементов шаблона отчёта (реф. KReportTemplateItem/Manager, X-2600).
// Загружается из XML прошивки:
//   .../mainapp/patient/report/config/TempletInfo.xml   — список шаблонов
//   .../report/template/ReportTemplateNP-<L>.xml        — порядок SubContent
//   .../report/template/SubContent/<name>.xml           — дерево <Content>/<Item>
// Атрибуты Item: Name/Title/Type/ShowTitle/Column/DataSrc.
//   Type ∈ RT_TITLE_TABLE_BLOCK | RT_TABLE_BLOCK | RT_TEXT_BLOCK | RT_IMAGE_BLOCK
//   DataSrc = "<источник>,<поле>" (напр. RT_DATASOURCE_PATIENT,RT_PATIENT_NAME).
struct ReportItem {
    QString name;
    QString title;      // ключ перевода (TR_*)
    QString type;       // RT_*_BLOCK
    QString dataSrc;    // "SOURCE,FIELD" или пусто
    int     column = 1;
    bool    showTitle = false;
    QVector<ReportItem> children;

    // Раскладка из секции <ItemConfig> (реф. KReportEditUIConfig): сопоставляется
    // по пути элемента ("/A/B/C"). imageWidth (px), alignH (Center/Left/Right),
    // fontType (ThirdTitle…), section (Header/Body), lineHeight1.
    int     imageWidth = 0;
    QString alignH;
    QString fontType;
    QString section;
    int     lineHeight = 0;

    QString dataSource() const;   // часть до запятой
    QString dataField() const;    // часть после запятой
    bool isImage() const { return type == "RT_IMAGE_BLOCK"; }
    bool isText()  const { return type == "RT_TEXT_BLOCK"; }
};

// Менеджер шаблонов отчётов (реф. KReportTemplateManager/KTemplateCfg).
class KReportTemplateManager
{
public:
    // Корень report/ (…/mainapp/patient/report). По умолчанию — из KSystem.
    explicit KReportTemplateManager(const QString &reportRoot = QString());

    // Список шаблонов из TempletInfo.xml (реф. LoadTempletInfo).
    QStringList TemplateNames() const;

    // Загрузить шаблон по имени ("NP-2x2") → плоско-упорядоченные корневые Item'ы
    // всех его SubContent-блоков (реф. LoadTemplate → resolve Ref → parse Content).
    QVector<ReportItem> LoadTemplate(const QString &name) const;

private:
    QVector<ReportItem> loadSubContent(const QString &fileName) const;
    QString reportRoot_;
};
