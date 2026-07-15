#pragma once

#include <QString>
#include <QStringList>

// Слой совместимости эндоскопов/камер (реф. KEncStyle, X-2600 — matched-scope часть).
// Читает <style>/<series>/<brand>/scope/matchedScope.ini: для каждой модели продукта
// (секции [Default]/[X-2600]/[X-2600S]/[X-2600A]/[X-2600B]) списки поддержанных
// эндоскопов (`scope=A,B,C…`) и камер (`camera=X,Y…`). Секция выбирается по модели
// продукта (реф. KSystemSet::GetProductModel), фолбэк — [Default].
//
// Off-device-ядро: парсинг ini + сопоставление строк (case-sensitive). Отложено
// (device — живой эндоскоп): getIsDefaultMatch (video.ini + ConvertSrc2Enc(GetEndoModel)),
// нормализация суффикса "*N" (KEndoScope::IsEndoModelHaveSuffix), кросс-проверка
// IsScopeValid с глобальным enc-списком getScopeList (genc/cenc/…enc.ini).
class KEncStyle
{
public:
    // Загрузка matchedScope.ini напрямую по пути.
    bool Load(const QString &matchedScopeIniPath);
    // Загрузка по (серия, бренд) — путь строится через KStyleConfig::GetStylePath.
    bool LoadForStyle(const QString &series, const QString &brand);

    // Модель продукта = имя секции (реф. GetProductModel). По умолчанию X-2600.
    void    SetProductModel(const QString &model) { productModel_ = model; }
    QString ProductModel() const { return productModel_; }

    // Списки из секции [Model] с фолбэком на [Default] (реф. 1:1, имена+опечатка).
    QStringList getSupportedScopeList() const;    // [Model]/scope | [Default]/scope
    QStringList GetSupprotedCameraList() const;   // [Model]/camera | [Default]/camera

    // Валидация членством в поддержанном списке (case-sensitive, реф. Qt::CaseSensitive).
    // IsScopeValid нормализует "…LT…" c "EC" → "…L/T…" (чистая строка); кросс-проверка
    // с глобальным enc-списком и "*N"-суффикс (device) — отложены.
    bool IsScopeValid(const QString &scope) const;
    bool IsCameraValid(const QString &camera) const;

    bool IsLoaded() const { return !iniPath_.isEmpty(); }

private:
    QStringList listFor(const QString &key) const;   // <Model>/<key> | Default/<key>

    QString iniPath_;
    QString productModel_ = "X-2600";
};
