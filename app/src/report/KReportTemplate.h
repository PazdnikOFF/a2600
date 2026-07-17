#pragma once

#include "report/KSysReportTempletCfg.h"   // KTempletBaseInfo (каталоги шаблонов)

#include <QString>
#include <QStringList>
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
class KTemplateCfg;
class KTemplateLibCfg;
class KRTDataSourceDemo;
class KRTDataSourceReal;

// Капстоун отчётного модуля. СОВМЕЩАЕТ ДВА API:
//  1) НАШ упрощённый загрузчик (ctor(reportRoot) + TemplateNames/LoadTemplate) — используется
//     существующими self-test'ами, оставлен как есть;
//  2) ФЕЙТФУЛ-API референса (KReportTemplateManager, sizeof 0x90, полиморфный из-за virtual dtor,
//     без базы): синглтон GetInstance + InitModule/UninitModule + геттеры частей.
// Реф. GetInstance = heap shared_ptr + std::call_once; здесь Meyers-static (эквивалентно).
// GetInstance НЕ вызывает InitModule (реф.) — вызыватель делает это сам.
class KReportTemplateManager
{
public:
    // --- наш упрощённый загрузчик (существующий API) ---
    // Корень report/ (…/mainapp/patient/report). По умолчанию — из KSystem.
    explicit KReportTemplateManager(const QString &reportRoot = QString());
    virtual ~KReportTemplateManager();

    // Список шаблонов из TempletInfo.xml (реф. LoadTempletInfo).
    QStringList TemplateNames() const;

    // Загрузить шаблон по имени ("NP-2x2") → плоско-упорядоченные корневые Item'ы
    // всех его SubContent-блоков (реф. LoadTemplate → resolve Ref → parse Content).
    QVector<ReportItem> LoadTemplate(const QString &name) const;

    // --- фейтфул-API референса ---
    static KReportTemplateManager *GetInstance();

    // Идемпотентно (гейт m_bInited): первичная провизия RO→userpreset (CopyDirectoryFiles),
    // загрузка каталогов (KSysReportTempletCfg), создание частей + Check/LoadCache. Возврат 1.
    int InitModule();
    // delete частей + m_bInited=false (векторы НЕ чистит — как реф.). Возврат 1.
    int UninitModule();

    KTemplateCfg      *GetTemplateCfg() const;       // assert non-null (реф.)
    KTemplateLibCfg   *GetTemplateLibCfg() const;    // assert non-null
    KRTDataSourceDemo *GetDemoDataSource() const;    // assert non-null
    KRTDataSourceReal *GetDataSourceReal() const;    // null-safe (без assert, реф.)

    const QVector<KTempletBaseInfo> &GetTempletsInfos() const { return m_vecTempletInfos; }
    const QVector<KTempletBaseInfo> &GetTempletLibInfos() const { return m_vecTempletLibInfos; }
    bool IsInited() const { return m_bInited; }

private:
    QVector<ReportItem> loadSubContent(const QString &fileName) const;
    void InitTempletsInfos();      // KSysReportTempletCfg → m_vecTempletInfos
    void InitTempletLibInfos();    // KSysReportTempletCfg → m_vecTempletLibInfos

    QString reportRoot_;

    bool                m_bInited = false;            // реф. +0x08
    KTemplateCfg       *m_pTemplateCfg = nullptr;     // +0x10
    KTemplateLibCfg    *m_pTemplateLibCfg = nullptr;  // +0x18
    KRTDataSourceDemo  *m_pDemoDataSource = nullptr;  // +0x20
    KRTDataSourceReal  *m_pDataSourceReal = nullptr;  // +0x28
    QVector<KTempletBaseInfo> m_vecTempletInfos;      // +0x60
    QVector<KTempletBaseInfo> m_vecTempletLibInfos;   // +0x78
};
