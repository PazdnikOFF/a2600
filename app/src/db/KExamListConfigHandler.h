#pragma once

#include <QString>

// Конфигурация видимости колонок списка осмотров (реф. KExamListConfigHandler,
// синглтон GetInstance, X-2600). Пользователь выбирает, какие столбцы показывать
// в списке осмотров/пациентов (возраст/заявитель/эндоскоп/дата/… + self-define
// поля) + настройки экспорта. Персист — QSettings-ini (генерится в рантайме).
// Имена методов 1:1 с оригиналом (IsShow<Col>/SetIsShow<Col>/SaveConfig).
class KExamListConfigHandler
{
public:
    static KExamListConfigHandler &GetInstance();

    void SetConfigFile(const QString &path) { cfgFile_ = path; load(); }

    // --- Видимость колонок (реф. IsShow*/SetIsShow*) ---
    bool IsShowAge() const            { return get("Age", true); }
    void SetIsShowAge(bool v)         { set("Age", v); }
    bool IsShowSexInfo() const        { return get("SexInfo", true); }
    void SetIsShowSexInfo(bool v)     { set("SexInfo", v); }
    bool IsShowBirthday() const       { return get("Birthday", false); }
    void SetIsShowBirthday(bool v)    { set("Birthday", v); }
    bool IsShowApplicant() const      { return get("Applicant", true); }
    void SetIsShowApplicant(bool v)   { set("Applicant", v); }
    bool IsShowEndoScope() const      { return get("EndoScope", true); }
    void SetIsShowEndoScope(bool v)   { set("EndoScope", v); }
    bool IsShowEndoScopeSN() const    { return get("EndoScopeSN", false); }
    void SetIsShowEndoScopeSN(bool v) { set("EndoScopeSN", v); }
    bool IsShowExamDate() const       { return get("ExamDate", true); }
    void SetIsShowExamDate(bool v)    { set("ExamDate", v); }
    bool IsShowRegisterNumber() const { return get("RegisterNumber", true); }
    void SetIsShowRegisterNumber(bool v) { set("RegisterNumber", v); }
    bool IsShowSickbedNum() const     { return get("SickbedNum", false); }
    void SetIsShowSickbedNum(bool v)  { set("SickbedNum", v); }
    bool IsShowTelephone() const      { return get("Telephone", false); }
    void SetIsShowTelephone(bool v)   { set("Telephone", v); }

    // Обобщённый запрос (реф. GetColumnIsShow).
    bool GetColumnIsShow(const QString &column) const { return get(column, false); }

    // Экспорт (реф. GetExportType/SetExportPath).
    int  GetExportType() const        { return get("ExportType", 0); }
    void SetExportType(int v)         { set("ExportType", v); }
    QString GetExportPath() const;
    void SetExportPath(const QString &p);

    void SaveConfig();   // реф. SaveConfig (QSettings sync)

private:
    KExamListConfigHandler() = default;
    QString cfgFile() const;
    void load() {}   // QSettings читает лениво
    bool get(const QString &key, bool def) const;
    int  get(const QString &key, int def) const;
    void set(const QString &key, bool v);
    void set(const QString &key, int v);
    QString cfgFile_;
};
