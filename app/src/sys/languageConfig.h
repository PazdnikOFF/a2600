#pragma once

#include <QObject>
#include <QString>

// Тип языка UI (реф. enum _KLanguageType, X-2600). Значения — 1-based, порядок
// реверснут из qm-таблицы бинарника (.rodata @0x886d70, шаг 0x38, индекс по типу):
// cn/en/sp/it/fr/ge/ru/pl. LanguageType=1 в mutilanguageinfo.ini соответствует
// китайскому (заводской дефолт). Корейский/португальский присутствуют как .qm
// (ko/po), но в декодированной таблице SetSystemLanguage их не было — их значения
// добавлены в хвост (порядок не подтверждён дизасмом).
enum _KLanguageType {
    KLangChinese    = 1,
    KLangEnglish    = 2,
    KLangSpanish    = 3,
    KLangItalian    = 4,
    KLangFrench     = 5,
    KLangGerman     = 6,
    KLangRussian    = 7,
    KLangPolish     = 8,
    KLangKorean     = 9,   // .qm есть (ko), позиция в таблице не подтверждена
    KLangPortuguese = 10,  // .qm есть (po), позиция в таблице не подтверждена
};

// Конфиг мультиязычности (реф. синглтон languageConfig, X-2600). Читает
// system/platform/mutilanguageinfo.ini: [MutiLanguageInfo] LanguageType/
// CurrentLanguage + [GooglePath] path/tabpath (путь к kchinesePunct.tab для
// китайского пунктуационного ввода). Держит текущий/сконфигурированный язык
// и пути; в оригинале — QObject с сигналом CurrentLanChange.
//
// Поля повторяют бинарник: currentLanguage_(+0x10), languageType_(+0x14),
// googlePath_(+0x18), puctPath_(+0x20).
class languageConfig : public QObject
{
    Q_OBJECT
public:
    static languageConfig &getInstance();

    // Загрузка mutilanguageinfo.ini (по умолчанию — из KSystem::SystemPath()).
    void Init(const QString &iniPath = QString());

    // Геттеры (реф. имена и офсеты полей).
    _KLanguageType getLanguageType()    const { return static_cast<_KLanguageType>(languageType_); }
    _KLanguageType getCurrentLanguage() const { return static_cast<_KLanguageType>(currentLanguage_); }
    QString        getGooglePath()      const { return googlePath_; }
    QString        getPuctPath()        const { return puctPath_; }

    // Сеттеры (реф. 1:1, только in-memory — персист делает KSystemSet):
    //  • setLanguageType пишет значение в ОБА поля (currentLanguage_ и languageType_);
    //  • setCurrentLanguage меняет currentLanguage_ лишь если t==Chinese(1) или
    //    t совпадает со сконфигурированным languageType_ (иначе no-op).
    void setLanguageType(_KLanguageType t);
    void setCurrentLanguage(_KLanguageType t);

signals:
    void CurrentLanChange();

private:
    explicit languageConfig(QObject *parent = nullptr) : QObject(parent) {}
    QString cfgFile(const QString &given) const;

    int     currentLanguage_ = KLangChinese;   // +0x10
    int     languageType_    = KLangChinese;   // +0x14
    QString googlePath_;                        // +0x18
    QString puctPath_;                          // +0x20
};
