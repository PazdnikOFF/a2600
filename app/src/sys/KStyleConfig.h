#pragma once

#include <QString>
#include <QStringList>

// Конфигурация стилей/брендов (реф. KEncStyle::getStyleList/getCurrentStyle* +
// KStyleFactory + stylelist.ini, X-2600). Бренды перечислены в
// system/style/stylelist.ini [Style]StyleList (порядок фиксирован — сохраняется в MCU).
// Ассеты бренда: system/style/<SERIES>/<BRAND>/ (qss/scope/…). Для X-2600/РуСкейн
// бренд = PyCkeun (см. KUIDesktop/Theme). Модель эндоскопа/камеры — device (KEncStyle).
class KStyleConfig
{
public:
    static KStyleConfig &GetInstance();

    void SetStyleRoot(const QString &path) { styleRoot_ = path; }  // .../system/style

    bool Load();                                  // читает stylelist.ini
    QStringList GetStyleList() const { return styleList_; }        // доступные бренды
    bool IsStyleValid(const QString &brand) const { return styleList_.contains(brand); }

    // Путь ассетов бренда: <styleRoot>/<series>/<brand> (реф. getCurrentStylePath).
    QString GetStylePath(const QString &series, const QString &brand) const;
    // Текущий бренд (по умолчанию — первый из списка; на устройстве — из настроек).
    QString GetCurrentStyle() const { return current_.isEmpty() && !styleList_.isEmpty()
                                              ? styleList_.first() : current_; }
    void SetCurrentStyle(const QString &brand) { current_ = brand; }

private:
    KStyleConfig() = default;
    QString styleRoot() const;
    QString styleRoot_;
    QStringList styleList_;
    QString current_;
};
